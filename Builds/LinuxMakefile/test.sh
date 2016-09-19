#!/bin/bash

VST=~/.vst/jgc/
mkdir -p "$VST"
make && cp build/stangin.so "$VST" && juce-host stangin.filtergraph
