# DAXPY : y = ax + y

## Work : 2N

= (N (ax) + N (ax + y)) * 2 (FLOPs per Double operation) = 4N

## Bytes : 24N Bytes + 8 Bytes

* Write 
= N (write y) * 8 (Bytes per Double) = 8N Bytes

* Read : Qr
= (2N (read x + read y) + 1 (read a)) * 8 (Bytes per Double) = 16N Bytes + 8 Bytes

* Total = 8N Bytes (Read) + 16N Bytes + 8 Bytes (Write) = 24N Bytes + 8 Bytes

## FLOPs/Byte = 4N / (24N + 8) ~= 1/6

# DGEMM : y = aAB + bC

## Work :

= (N^2 (AB) * N (aAB) + N^2 (bC) + N^2 (aAB + bC)) (2 FLOPs per Double operation) = 2N^3 + 4N^2

## Bytes :

* Write = N^2 (write y) * 8 (Bytes per Double) = 8N^2

* Read = (3N^2 (read A + read B + read C) + 2 (read a + read b)) * 8 (Bytes per Double) = 24 N^2 + 16

* Total = 8N^2 + 24N^2 + 16 = 32N^2 + 16

## FLOPs/Byte = (2N^3 + 4N^2) / (32N^2 + 16) ~= N/16

# SGEMV : y = aAx + by

## Work :

= (N+N-1 (Ax) * N(a * Ax)) + N(by) + N(aAx + by) = (2N - 1)*N + 2N = 2N^2 - N + 2N = 2N^2 + 2N

## Bytes :

* Write :

= N (write y) * 4(Bytes per Float)

* Read :

= (N^2 (Read A) + 2N(Read x + Read y) + 2 (read a + read b)) * 4(Bytes per Float) = 4N^2 + 8N + 8

* Total = 4N^2 + 12N + 8

## FLOPs/Byte = (2N^2 + 2N) / (2N^2 + 12N + 8) ~= 1/2


