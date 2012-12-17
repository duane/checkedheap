import sys
import pickle

modes, results = pickle.load(sys.stdin)
for prog_n, times in results.iteritems():
  # normalize by dividing every item by the first one.
  mu0, sigma0 = times[0]
  times = [(mu / mu0, sigma / mu0) for (mu, sigma) in times]
  results[prog_n] = times
pickle.dump((modes, results), sys.stdout)

