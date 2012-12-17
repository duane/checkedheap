import sys
import pickle

(modes, data) = pickle.load(sys.stdin)

plots = {}

for prog_name,times in data.iteritems():
  # generate signature
  sig = 0
  for idx, (mu, sigma) in enumerate(times):
    if mu > 0.0:
      sig |= (1 << idx)
  # now store the data in the appropriate plot
  if sig in plots:
    data = plots[sig]
  else:
    data = []
  data.append((prog_name, times))
  plots[sig] = data

n = 0
for sig, plot in plots.iteritems():
  name = "plot%d.data" % n
  n += 1
  with open(name, "w") as out:
    names = ["\"%s\" \"%s\"" % (name, name) for (idx, name) in enumerate(modes) if sig & (1 << idx) != 0]
    out.write("title %s\n" % ' '.join(names))
    for prog_name, times in plot:
      times_str = ' '.join(["%0.3f %0.3f" % (mu, sigma) for idx,(mu, sigma) in enumerate(times) if sig & (1 << idx) != 0])
      out.write("\"%s\" %s\n" % (prog_name, times_str))

