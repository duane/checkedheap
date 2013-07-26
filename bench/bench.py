import os
import pickle
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
  var = sum([pow(mu - sample, 2) for sample in samples])/float(times-1)
  sigma = sqrt(var)
  return (mu, sigma)

if __name__ == '__main__':
  dyld = lambda lib: {"DYLD_INSERT_LIBRARIES" : lib, "DYLD_FORCE_FLAT_NAMESPACE" : ""}

  modes = [
    ("vanilla", {}, True),
    ("opt", dyld("../libcheckedheap.dylib"), False),
    ("thread", dyld("../libcheckedheap_thread.dylib"), True),
  ]

  progs = [
    (False, ["./malloc-verifier", "1", "5"], 10),
    (True, ["./malloc-verifier", "2", "5"], 10),
    (True, ["./malloc-verifier", "4", "5"], 10),
    (False, ["./stress", "128"], 10),
    (False, ["./stress", "1024"], 10),
    (False, ["./binary", "5"], 100),
    (False, ["./binary", "10"], 10),
    (False, ["./binary", "15"], 5),
  ]

  with open("test_data.dict", "wb") as f:
    names = [mode[0] for mode in modes]
    #file.write('Title %s\n' % ' '.join(names))
    #file.flush()
    results = {}
    for prog_parallel, prog, times in progs:
      time_result = []
      prog_name = ' '.join(prog)
      sys.stdout.write("%s:" % prog_name)
      sys.stdout.flush()
      #file.write("\"%s\"" % ' '.join(prog))
      for name, env, mode_parallel in modes:
        if prog_parallel and not mode_parallel:
          time_result.append((0.0, 0.0))
          #file.write(" 0.000")
        else:
          timing_data(proc(env, *prog), times=times)
          (mu, sigma) = timing_data(proc(env, *prog), times=times)
          time_result.append(timing_data(proc(env, *prog), times=times))
          #file.write(" %0.3f" % mu)
      results[prog_name] = time_result
      sys.stdout.write("ok.\n")
      sys.stdout.flush()
    data = ([mode[0] for mode in modes], results)
    pickle.dump(data, f)

