# Go Sources

To bring this to compliance with the AGPLv3, I'm adding these files here (as the original repository linked is now gone).

These are the unmodified files from my own repository, including some comments likely missing from the public one.  The comments are inaccurate, and were written for *mostly* my own work.  Since the original repository was public (prior to a run in with IOI), I commented the code fairly heavily.

## Notes

These notes serve as a correction so I don't need to modify the branched code.

### Project Colander (Colander/Collander)

Neither `colander` (C variant) nor `collander` (OpenCL variant) are public code, and they were never tied into this code in a way that binds them to the AGPLv3.  Their purpose is to attack known block ciphers using various methods.  The XTEA was misnamed due to an offset difference in the list (the joys of dev tools instead of actually making this all sane).  They are lists of files maintained against keyed names.  The names had an offset issue, misidentifying by one.  (All versions of the application contain variants of things like XTEA and XXTEA, inverting certain functions that are known to not break the algorithm's ciphering; ironically it was an XTEA variant being disabled in the naming list that caused this.)

Colander also serves as a hash test, which is how the CRC32 component was detected once the region was isolated (though this was a trial and error fix).

It was only when I ported the code from Go to Rust that I realised the error Collander had made (`golander`, the Go version, contains Go sources of all hashes which are blindly copied on a hit using a format function with keys the list; `colander` and `collander` similarly maintain C and OpenCL variants).

### XTEA Key

The comments make implications about the origins of the XTEA key which aren't true.

 * `collander`'s offset being off in the string table means I've attributed the access to the Yarrkov attack, which isn't true (it was collander's `XXTEA` vector).  The actual algorithm used was a combination of attacks, mostly derived from the commonly known attacks and [Bitsum Attacks](https://thescipub.com/PDF/jcssp.2014.1077.1083.pdf);
 * From there I received third party confirmation from people about the key.  This included someone who claimed to have memory dumped the game and pulled the key from it that way.

 ### LOCR

 The LOCR was a by-hand decipher against game localisation strings and Absolution strings.  Absolution's fragments are what actually gave it away.

 The Rust version and C version of the Hitman code prebuild this library and shipped it prebuild.  The Go version built as part of the module launch (which added to the cost of running `h6xxtea` despite the fact the code was never used).  For this purpose the code is included, despite my reservations about making a working implementation public.

 After all, despite this all being my code, GPL compliance is GPL compliance, even if I'm not the one who violated it.