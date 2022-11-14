#!usr/bin/perl

# crypt and calculate hashes for a given string
# (util program for swspr)
# by Zlatko Karakas, 29.05.2004.

print "Gimme string to hash:\n";
while (<STDIN>) {
    chomp;
    @_ = split //, $_;
    for ($i = $h1 = $h2 = 0; $i < length $_; $i++) {
        $h1 += $i + ord $_[$i];
        $h1 <<= 1;
        $h1 ^= $i + ord $_[$i];
        $h2 += ord($_[$i]) * $i * $i;
    }
    print "Hash1: $h1\nHash2: $h2\n";
    for ($i = 0; $i < length $_; $i++) {
        $out = ord($_[$i]) * 21;
        $out ^= 0x5a;
        printf "0x%02x, ", $out % 256;
    }
}