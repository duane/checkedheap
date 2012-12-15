import os
import sys
import time
from math import sqrt
from subprocess import Popen


def proc(env, *args):
  dev_null = open("/dev/null", "w")
  child_env = dict(os.environ.iteritems())
  for key, val in env.items():
    child_env[key] = val
  def wrapper():
    popen = Popen(args, stdout=dev_null, stderr=sys.stderr, env=child_env)
    status = popen.wait()
    assert status == 0
  return wrapper

# returns (mu, sigma)
def timing_data(func, times=10):
  samples = [0] * times
  for n in range(times):
    t1 = time.time()
    res = func()
    t2 = time.time()
    s = t2 - t1
    samples[n] = s
  mu = sum(samples)/float(times)
  var = sum([pow(mu - sample, 2) for sample in samples])/float(times)
  sigma = sqrt(var)
  return (mu, sigma)

if __name__ == '__main__':
  dyld = lambda lib: {"DYLD_INSERT_LIBRARIES" : lib, "DYLD_FORCE_FLAT_NAMESPACE" : ""}

  modes = [
    ("vanilla", {}),
    ("opt", dyld("../libcheckedheap.dylib")),
    ("thread", dyld("../libcheckedheap_thread.dylib")),
  ]

  progs = [
    #(["./stress", "128"], 10),
    #(["./stress", "2048"], 10),
    #(["./stress", "65536"], 5),
    (["./binary", "5"], 1),
    #(["./binary", "10"], 10),
    #(["./binary", "15"], 5),
  ]

  file = sys.stdout

  names = [mode[0] for mode in modes]
  file.write('Title %s\n' % ' '.join(names))
  file.flush()
  for prog, times in progs:
    file.write("\"%s\"" % ' '.join(prog))
    for name, env in modes:
      timing_data(proc(env, *prog), times=times)
      file.write(" %0.3f %0.3f" % timing_data(proc(env, *prog), times=times))
    file.write("\n")
    file.flush()

