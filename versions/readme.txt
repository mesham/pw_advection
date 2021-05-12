One - Initial version, all looks OK but limited in data size due to SB and external data access is in 64 bit mode only
Two - As one but 512 width ports (all still same HBM port)
Three - As two but extended stream FIFO depth from 1 to 4
Four - As three but FIFO depth 16
Five - Same as three but HBM ports split out
Six - Chunking up domain into sub-chunks to reduce BRAM usage
Seven - Connected to DRAM