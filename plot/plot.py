import numpy as np
import matplotlib.pyplot as plt

device_name = 'RX480'
data = [
    {
        'name': 'DEVICE_LOCAL_MEMORY 1',
        'mean': 0.632939,
        'stddev': 0.085272,
        'samples': 6000,
    },
    {
        'name': 'DEVICE_LOCAL_MEMORY 0',
        'mean': 0.623012,
        'stddev': 0.085789,
        'samples': 6000,
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
ax.set_title('Vulkan Breakout Timestamp Benchmark - {}'.format(device_name))
ax.yaxis.grid(True)

plt.tight_layout()
plt.savefig('plot.svg')
plt.show()
