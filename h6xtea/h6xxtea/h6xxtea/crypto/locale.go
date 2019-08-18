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

// Table for converting a Raw Byte to a Ciphered Byte.
var LocaleByteToCipher map[byte]byte

// Table for converting a Ciphered Byte to a Raw Byte.
var LocaleCipherToByte map[byte]byte = buildCipherKey()

// This is actually stupidly expensive, so build tables.
func buildCipherKey() map[byte]byte {
	LocaleByteToCipher = make(map[byte]byte)
	cipherToByte := make(map[byte]byte)
	var a byte
	for i := 0; i < 256; i++ {
		a = byte(DecipherChar(byte(i)))
		cipherToByte[byte(i)] = a
		LocaleByteToCipher[a] = byte(i)
	}
	return cipherToByte
}

// DecipherChar deciphers a byte using the algorithm;
// please use the tables (CipherToByte and ByteToCipher) instead.
func DecipherChar(b uint8) (r uint8) {
	r = 0x00

	// bit 0 -> bit 0
	r |= b & 1

	// Shift down to make it easier
	b = b >> 1

	// bit 1 -> bit 4
	r |= (b & 1) << 4

	// Shift down to make it easier
	b = b >> 1

	// !bit 2 -> bit 1
	r |= (^b & 1) << 1

	// Shift down to make it easier
	b = b >> 1

	// !bit 3 -> bit 5
	r |= (^b & 1) << 5

	// Shift down to make it easier
	b = b >> 1

	// bit 4 -> bit 2
	r |= (b & 1) << 2

	// Shift down to make it easier
	b = b >> 1

	// bit 5 -> bit 6
	r |= (^b & 1) << 6

	// Shift down to make it easier
	b = b >> 1

	// bit 6 -> bit 3
	r |= (b & 1) << 3

	// Shift down to make it easier
	b = b >> 1

	// bit 7 = !bit 7
	r |= (^b & 1) << 7

	return r
}
