/**
 * Hitman (Hitman 2016) Go Modification Backend
 * https://github.com/awstanley/hitman
 *
 * Copyright (C) 2016 A.W. 'Swixel' Stanley <code@swixel.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

package crypto

import (
	"bytes"
	"encoding/binary"
	"hash/crc32"
	"os"
)

// Note: Collander mis-hit, but whatever...
// COLLANDER HIT: XXTEA
//                                  Signing bias.
//                                  |
// XXTEA: 0x9e3779b9 ^ 0xffffffff + 0x00000001 = 0x61C88647
//                                               |
//                                               Delta.
var xxTeaDelta uint32 = 0x61C88647
var xxTeaSum uint32 = 0xC6EF3720

// packagedefinition and thumbs.dat use the following header:
// 0x22, 0x3D, 0x6F, 0x9A, 0xB3, 0xF8, 0xFE, 0xB6,
// 0x61, 0xD9, 0xCC, 0x1C, 0x62, 0xDE, 0x83, 0x41
//
// What this means is presently unknown and but it is likely tied
// to the keys they both share:
// 0x30F95282, 0x1F48C419, 0x295F8548, 0x2A78366D
//
// These keys were found by blindly scanning a memory dump of the game
// binary (i.e. searching for a matching hex string) and then using a
// delta matching various types.  The algorithm was brute-forced out
// stepping up/down using some old C code (not in this repository).
//
// By nature of how the game works the keys will always be visible,
// if they move them, or change them, Yarrkov's vector can be used
// provided the encryption type does not.  Failing that there are other
// ways to get hold of them.

// EncryptBlockXXTEA encrypts a block using a given key and the game specific
// delta and sum values.
//
// A block is comprised of two int32s (taken as pointers).
// There is no return value (it modifies the input values).
func EncryptBlockXXTEA(A *int32, B *int32, keys []int32) {

	a := uint32(*A)
	b := uint32(*B)
	xxSum := uint32(0)

	for i := 0; i < 32; i++ {
		a += uint32(int32(((b<<4)^(b>>5))+b) ^ (int32(xxSum) + keys[xxSum&3]))
		xxSum -= xxTeaDelta
		b += uint32(int32(((a<<4)^(a>>5))+a) ^ (int32(xxSum) + keys[(xxSum>>11)&3]))
	}

	*A = int32(a)
	*B = int32(b)
}

// DecryptBlockXXTEA decrypts a block using a given key and the game specific
// delta and sum values.
//
// A block is comprised of two int32s (taken as pointers).
// There is no return value (it modifies the input values).
func DecryptBlockXXTEA(A *int32, B *int32, keys []int32) {
	a := uint32(*A)
	b := uint32(*B)
	xxSum := xxTeaSum

	for i := 0; i < 32; i++ {
		b -= uint32(int32(((a<<4)^(a>>5))+a) ^ (int32(xxSum) + keys[(xxSum>>11)&3]))
		xxSum += xxTeaDelta
		a -= uint32(int32(((b<<4)^(b>>5))+b) ^ (int32(xxSum) + keys[xxSum&3]))
	}

	*A = int32(a)
	*B = int32(b)
}

// EncryptFileXXTEA takes a source name and a destination name, attempting to convert the source
// into an encrypted destination.  It also requires header bytes (theoretically optional, but
// in reality required for the game to read it) and keys (also required).  At present there
// appear to be common header and keys (listed in the source at the top of xxtea.go).
//
// The CRC32 will be automatically added prior to the data.  If you do not need the CRC32 value
// you should use the buffer encryption (ExcryptXXTea).
//
// On failure it will return an error (though these will be file related; the encryption system
// has no way of knowing if it worked or not).
func EncryptFileXXTEA(sourceFile string, destFile string, header []byte, keys []int32) error {
	src, err := os.Open(sourceFile)
	if err != nil {
		return err
	}
	// Close whenever we exit
	defer src.Close()

	// Get the information to get the size
	stat, err := src.Stat()
	if err != nil {
		return err
	}

	// Size
	sz := stat.Size()

	// Output (will overwrite)
	dst, err := os.Create(destFile)
	if err != nil {
		return err
	}
	// Close whenever we exit
	defer dst.Close()

	// Allocate a source buffer
	sBuf := make([]byte, sz)

	// Read everything
	src.Read(sBuf)

	// Use the internal encrypt function
	dBuf := EncryptBufferXXTEA(sBuf, keys)

	// Write the header
	dst.Write(header)

	// Write the IEEE CRC32.
	binary.Write(dst, binary.LittleEndian, crc32.ChecksumIEEE(sBuf))

	// Write the data (the body)
	dst.Write(dBuf)

	// Return nil (no error)
	return nil
}

// DecryptFileXXTEA decrypts from source to dest, stripping the header (of length 'headerLen').
// It requires keys to be passed as an array; these should be 4 int32s.
//
// The header length MUST contain anything unencrypted (typically it will be 20, 16 bytes for
// the header +4 for the CRC32).
//
// On failure it will return an error (though these will be file related; the decryption system
// has no way of knowing if it worked or not).
func DecryptFileXXTEA(sourceFile string, destFile string, headerLength int, keys []int32) error {
	// Open the source file
	src, err := os.Open(sourceFile)
	if err != nil {
		return err
	}
	// Close when done
	defer src.Close()

	// Use stat to get the information
	stat, err := src.Stat()
	if err != nil {
		return err
	}

	// Get the size from the stat call.
	// (This includes the header)
	sz := stat.Size()

	// Open the destination
	dst, err := os.Create(destFile)
	if err != nil {
		return err
	}
	// Close when done
	defer dst.Close()

	// Seek past the header
	src.Seek(int64(headerLength), 0)

	// Allocate memory sufficient to hold it all
	sBuf := make([]byte, sz-int64(headerLength))

	// Read it
	src.Read(sBuf)

	// Decrypt it
	dBuf := DecryptBufferXXTEA(sBuf, keys)

	// Store it (nothing fancy)
	dst.Write(dBuf)

	return nil
}

// DecryptBufferXXTEA decrypts bytes from one array into another.
// There is no errore turn as nothing can really go wrong.
//
// Note: the header should not be included here, this is just XXTea.
func DecryptBufferXXTEA(sourceBuffer []byte, keys []int32) []byte {
	// Create writers to make this easier
	src := bytes.NewReader(sourceBuffer)
	dst := bytes.NewBuffer(nil)

	// xxTea's block size should be 8 (two uint32s)
	blockCount := len(sourceBuffer) / 8

	// This is done this way in the event the XBox or PS4 variants use
	// a different endianness (as is likely/common).
	var a int32
	var b int32
	for i := 0; i < blockCount; i++ {
		binary.Read(src, binary.LittleEndian, &a)
		binary.Read(src, binary.LittleEndian, &b)
		DecryptBlockXXTEA(&a, &b, keys)
		binary.Write(dst, binary.LittleEndian, &a)
		binary.Write(dst, binary.LittleEndian, &b)
	}

	rawDst := dst.Bytes()
	rawLen := len(rawDst)
	i := 1

	// Strip padding
	for ; i < rawLen; i++ {
		if byte(rawDst[rawLen-i]) != byte(0x00) {
			i-- // don't delete the final non-null byte
			break
		}
	}

	// Force null trimming
	return rawDst[0 : rawLen-i]
}

// EncryptBufferXXTEA encrypts bytes from one array into another.
// There is no errore turn as nothing can really go wrong.
//
// Note: the header should not be included here, this is just XXTea.
func EncryptBufferXXTEA(sourceBuffer []byte, keys []int32) []byte {

	// This is an ugly pad loop.
	{
		for {
			if len(sourceBuffer)%8 == 0 {
				break
			} else {
				sourceBuffer = append(sourceBuffer, 0x00)
			}
		}
	}

	// xxTea's block size should be 8 (two uint32s)
	blockCount := len(sourceBuffer) / 8

	// Create writers to make this easier
	src := bytes.NewReader(sourceBuffer)
	dst := bytes.NewBuffer(nil)

	// This is done this way in the event the XBox or PS4 variants use
	// a different endianness (as is likely/common).
	var a int32
	var b int32
	for i := 0; i < blockCount; i++ {
		binary.Read(src, binary.LittleEndian, &a)
		binary.Read(src, binary.LittleEndian, &b)
		EncryptBlockXXTEA(&a, &b, keys)
		binary.Write(dst, binary.LittleEndian, &a)
		binary.Write(dst, binary.LittleEndian, &b)
	}
	return dst.Bytes()
}
