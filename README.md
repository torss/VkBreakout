# VkBreakout

This fork implements some smaller changes in the `master` branch:

- Computes the standard deviation in addition to the mean time values.
- Fixes the `timestampPeriod` bug _(see <https://github.com/khalladay/VkBreakout/issues/1#issue-474554203>)_.
- Cycles through brick colors _(just to show that the data is actually being updated)_.
- Adds a minimal Python plot generator script for the results _(`plot/plot.py`)_.
- Persistent samples _(currently serialized to `local/results[DLM-...][SF-...].ubjson`)_.
- Adds a `SKIP_FRAMES` test option, which however didn't change the results.

Current `torss`-fork results _(`master` branch)_:

<div align="center">
  <img width="512" src="https://raw.githubusercontent.com/torss/VkBreakout/master/plot/plot.svg?sanitize=true" alt="VkBreakout 'torss'-fork result plot" />
</div>
<div align="center">
  DLM & SF stand for the DEVICE_LOCAL_MEMORY & SKIP_FRAMES options.
</div>

I.e. nothing interesting to report so far, since there is no substantial performance difference between `#define DEVICE_LOCAL_MEMORY 0` and `1`, nor did the introduced `SKIP_FRAMES` change that.
`DEVICE_LOCAL_MEMORY` & `SKIP_FRAMES` reside in `Breakout/stdafx.h`, the Vulkan benchmark result code can be found at the bottom of `Breakout/Renderer.cpp`.

## Original repository README.md content

This is a quick (and not terribly fun) breakout game made to try out different parts of the Vulkan API. It's the first thing I've built with Vulkan, so please don't take it as an example of good code or project architecture, because it's awful. 

The project has multiple branches, representing different approaches to passing uniform data to shaders, and allocating memory. The branches marked with the prefix 01- are the branches used in my initial performance tests, presented [on my website](http://kylehalladay.com/blog/tutorial/vulkan/2017/08/13/Vulkan-Uniform-Buffers.html). 

The branches marked with 02 or 03 prefixes were used in the [follow up post](http://kylehalladay.com/blog/tutorial/vulkan/2017/08/30/Vulkan-Uniform-Buffers-pt2.html)

The performance of the 01- branches are as follows: 

![Graph](http://i.imgur.com/1TRVFSp.png)

The performance of the 03- branches are as follows:

![Graph](http://i.imgur.com/RDbSSP0.png)

More information about how this performance data was gathered, the reasoning behind the changes in each branch, and general implementation details can be found on the two linked blog posts. 
