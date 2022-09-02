/*
 * Sieve of Eratosthenes Example
 *
 * This example program finds prime numbers up to a limit using the
 * sieve of Eratosthenes.
 */

use List;

config var limit : uint = 1000; // maximum number to include in the sieve

proc main() {
  // list of confirmed primes
  var primes : list(uint);
  // range of potential prime numbers
  var potentialPrimes = 2..limit;
  // flags for known composite (sieved out) numbers
  var knownComposite : [potentialPrimes] bool = false;

  // repeatedly add the first potential prime to our primes list, then exclude
  // all following multiples
  for potentialPrime in potentialPrimes {
    if (!knownComposite[potentialPrime]) {
      primes.append(potentialPrime);
      for i in potentialPrime*2..limit by potentialPrime {
        knownComposite[i] = true;
      }
    }
  }

  writeln(primes);
  writeln("Count of primes up to ", limit, ": ", primes.size);
}
