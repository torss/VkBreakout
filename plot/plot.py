import numpy as np
import matplotlib.pyplot as plt

device_name = 'RX480'
sample_title = '10k'
data = [
    {
        'name': 'DLM 1, SF 0',
        'mean': 0.508311,
        'stddev': 0.017079,
        'samples': 10000,
        'color': 'xkcd:red',
    },
    {
        'name': 'DLM 0, SF 0',
        'mean': 0.512627,
        'stddev': 0.045577,
        'samples': 10000,
        'color': 'xkcd:red',
    },
    {
        'name': 'DLM 1, SF 100',
        'mean': 0.554869,
        'stddev': 0.098656,
        'samples': 10000,
        'color': 'xkcd:blue',
    },
    {
        'name': 'DLM 0, SF 100',
        'mean': 0.618867,
        'stddev': 0.076571,
        'samples': 10000,
        'color': 'xkcd:blue',
    },
    {
        'name': 'DLM 1, SF 1k',
        'mean': 0.646941,
        'stddev': 0.079635,
        'samples': 10000,
        'color': 'xkcd:green',
    },
    {
        'name': 'DLM 0, SF 1k',
        'mean': 0.649885,
        'stddev': 0.069215,
        'samples': 10000,
        'color': 'xkcd:green',
    }
]

names = list(map(lambda x: x['name'], data))
x_pos = np.arange(len(names))
means = list(map(lambda x: x['mean'], data))
errors = list(map(lambda x: x['stddev'], data))
colors = list(map(lambda x: x['color'], data))

fig, ax = plt.subplots(figsize=(8, 6))
ax.bar(x_pos, means, yerr=errors, align='center', alpha=0.5, ecolor='black',
       capsize=10, color=colors)
ax.set_ylabel('ms')
ax.set_xticks(x_pos)
ax.set_xticklabels(names)
ax.set_title('Vulkan Breakout Timestamp Benchmark - {} - {} samples'
             .format(device_name, sample_title))
ax.yaxis.grid(True)

plt.tight_layout()
plt.savefig('plot.svg')
plt.show()
