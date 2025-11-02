# A proof development tool for metamath

This is a prototype of proof development tool intended to work with metamath (https://us.metamath.org/), a minimalist formal language capable of expressing all the mathematical proofs ever created.

Recently there has been a spur of interest in proving mathematical theorems using the generative power of large language models (LLM). The emphasis of this project, however, is somewhat complementary. It aims to find out how far we can go **without** LLMs by taking the traditional approach. So far it has used a combination of Monte Carlo Tree Search and aggressive pruning.

Currently this project focuses on the part of the metamath database (set.mm) on propositional logic. Of the 1843 propositional theorems in the database, 1766 can be solved under 32k nodes, reaching an accuracy of 95.8%. Noteably, this algorithm does **not** use neural networks. Running single-threaded on a CPU, the whole process takes about 30s.
