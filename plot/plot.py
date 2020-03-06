import numpy as np
import matplotlib.pyplot as plt

device_name = 'RX480'
sample_title = '10k'
data = [
    {
        'name': 'DEVICE_LOCAL_MEMORY 1',
        'mean': 0.508311,
        'stddev': 0.017079,
        'samples': 10000,
    },
    {
        'name': 'DEVICE_LOCAL_MEMORY 0',
        'mean': 0.512627,
        'stddev': 0.045577,
        'samples': 10000,
    }
]

names = list(map(lambda x: x['name'], data))
x_pos = np.arange(len(names))
means = list(map(lambda x: x['mean'], data))
errors = list(map(lambda x: x['stddev'], data))

fig, ax = plt.subplots()
ax.bar(x_pos, means, yerr=errors, align='center', alpha=0.5, ecolor='black',
       capsize=10)
ax.set_ylabel('ms')
ax.set_xticks(x_pos)
ax.set_xticklabels(names)
ax.set_title('Vulkan Breakout Timestamp Benchmark - {} - {} samples'
             .format(device_name, sample_title))
ax.yaxis.grid(True)

plt.tight_layout()
plt.savefig('plot.svg')
plt.show()
