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

package main

import (
	"./crypto"
	"flag"
	"fmt"
	"os"
)

// Header used by `thumbs.dat` and `packagedefinition.txt`
var header = []byte{
	0x22, 0x3D, 0x6F, 0x9A, 0xB3, 0xF8, 0xFE, 0xB6,
	0x61, 0xD9, 0xCC, 0x1C, 0x62, 0xDE, 0x83, 0x41,
}

// Keys used by `thumbs.dat` and `packagedefinition.txt`
var keys = []int32{0x30F95282, 0x1F48C419, 0x295F8548, 0x2A78366D}

// header + CRC32 = 20 bytes
var headerLength = 20

func Usage() {

	fmt.Printf("Example usage:\n  %s -src=\"thumbs.dat\" -dst=\"thumbs.txt\" -d\n\n", os.Args[0])
	fmt.Println("Flags:")
	fmt.Println("  -src <src>       (required; source file)")
	fmt.Println("  -dst <dst>       (required; destination file)")
	fmt.Println("")
	fmt.Println("Operations (exactly one is required):")
	fmt.Println("  -e               (encrypt src->dst)")
	fmt.Println("  -d               (decrypt src->dst)")
}

func main() {
	encrypt := flag.Bool("e", false, "If used it will encrypt from src to dst.")
	decrypt := flag.Bool("d", false, "If used it will decrypt from src to dst.")
	src := flag.String("src", "", "Source file.")
	dst := flag.String("dst", "", "Destination (output).")
	flag.Parse()

	// Check it meets what's desired:
	if *src == "" || *dst == "" || (!*encrypt && !*decrypt) || (*encrypt && *decrypt) {
		Usage()
		return
	}

	if *encrypt {
		fmt.Printf("Encrypting %s to %s\n", *src, *dst)
		crypto.EncryptFileXXTEA(*src, *dst, header, keys)
		return
	}

	if *decrypt {
		fmt.Printf("Decrypting %s to %s\n", *src, *dst)
		crypto.DecryptFileXXTEA(*src, *dst, headerLength, keys)
		// return
	}

	// Nothing goes here ...
}
