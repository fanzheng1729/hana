# A proof development tool for metamath

This is a prototype of proof development tool intended to work with metamath (https://us.metamath.org/), a minimalist formal language capable of expressing all the mathematical proofs ever created.

Recently there has been a spur of interest in proving mathematical theorems using the generative power of large language models (LLM). The emphasis of this project, however, is somewhat complementary. It aims to find out how far we can go **without** LLMs by taking the traditional approach. So far it has used a combination of Monte Carlo Tree Search and aggressive pruning.

Currently this project focuses on the part of the metamath database (set.mm) on propositional logic. Of the 1843 propositional theorems in the database, 1766 can be solved under 32k nodes, reaching an accuracy of 95.8%. Noteably, this algorithm does **not** use neural networks. Running single-threaded on a CPU, the whole process takes about 30s.

Sample proof of the associative law of equivalence, found by the program at a budget of 128k nodes:

biimp          |- ( ( $\phi$ <-> $\psi$ ) -> ( $\phi$ -> $\psi$ ) ) 

com12         |- ( $\phi$ -> ( ( $\phi$ <-> $\psi$ ) -> $\psi$ ) ) 

pm5.1im       |- ( $\phi$ -> ( $\psi$ -> ( $\phi$ <-> $\psi$ ) ) ) 

impbid       |- ( $\phi$ -> ( ( $\phi$ <-> $\psi$ ) <-> $\psi$ ) ) 

bibi1d      |- ( $\phi$ -> ( ( ( $\phi$ <-> $\psi$ ) <-> $\chi$ ) <-> ( $\psi$ <-> $\chi$ ) ) ) 

biimpcd    |- ( ( ( $\phi$ <-> $\psi$ ) <-> $\chi$ ) -> ( $\phi$ -> ( $\psi$ <-> $\chi$ ) ) ) 

biimpr        |- ( ( $\psi$ <-> $\chi$ ) -> ( $\chi$ -> $\psi$ ) ) 

com12        |- ( $\chi$ -> ( ( $\psi$ <-> $\chi$ ) -> $\psi$ ) ) 

biimpr       |- ( ( $\phi$ <-> $\psi$ ) -> ( $\psi$ -> $\phi$ ) ) 

syl9r       |- ( ( $\phi$ <-> $\psi$ ) -> ( $\chi$ -> ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) ) ) 

2a1           |- ( $\phi$ -> ( -. $\chi$ -> ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) ) ) 

simplim         |- ( -. ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) -> ( $\psi$ <-> $\chi$ ) ) 

biimpcd        |- ( $\psi$ -> ( -. ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) -> $\chi$ ) ) 

con1d         |- ( $\psi$ -> ( -. $\chi$ -> ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) ) ) 

pm5.21ni     |- ( -. ( -. $\chi$ -> ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) ) -> ( $\phi$ <-> $\psi$ ) ) 

con1i       |- ( -. ( $\phi$ <-> $\psi$ ) -> ( -. $\chi$ -> ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) ) ) 

bija       |- ( ( ( $\phi$ <-> $\psi$ ) <-> $\chi$ ) -> ( ( $\psi$ <-> $\chi$ ) -> $\phi$ ) ) 

impbid    |- ( ( ( $\phi$ <-> $\psi$ ) <-> $\chi$ ) -> ( $\phi$ <-> ( $\psi$ <-> $\chi$ ) ) ) 

bicom      |- ( ( $\phi$ <-> $\psi$ ) <-> ( $\psi$ <-> $\phi$ ) ) 

pm5.1im         |- ( $\psi$ -> ( $\phi$ -> ( $\psi$ <-> $\phi$ ) ) ) 

com12          |- ( $\phi$ -> ( $\psi$ -> ( $\psi$ <-> $\phi$ ) ) ) 

biimpr          |- ( ( $\psi$ <-> $\phi$ ) -> ( $\phi$ -> $\psi$ ) ) 

com12          |- ( $\phi$ -> ( ( $\psi$ <-> $\phi$ ) -> $\psi$ ) ) 

impbid        |- ( $\phi$ -> ( $\psi$ <-> ( $\psi$ <-> $\phi$ ) ) ) 

bibi1d       |- ( $\phi$ -> ( ( $\psi$ <-> $\chi$ ) <-> ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) ) ) 

biimpd      |- ( $\phi$ -> ( ( $\psi$ <-> $\chi$ ) -> ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) ) ) 

simplim             |- ( -. ( ( $\psi$ <-> $\phi$ ) -> $\chi$ ) -> ( $\psi$ <-> $\phi$ ) ) 

biimpcd            |- ( $\psi$ -> ( -. ( ( $\psi$ <-> $\phi$ ) -> $\chi$ ) -> $\phi$ ) ) 

con1d             |- ( $\psi$ -> ( -. $\phi$ -> ( ( $\psi$ <-> $\phi$ ) -> $\chi$ ) ) ) 

com3r            |- ( ( $\psi$ <-> $\phi$ ) -> ( $\psi$ -> ( -. $\phi$ -> $\chi$ ) ) ) 

2a1              |- ( $\chi$ -> ( $\psi$ -> ( -. $\phi$ -> $\chi$ ) ) ) 

pm5.21ni        |- ( -. ( $\psi$ -> ( -. $\phi$ -> $\chi$ ) ) -> ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) ) 

con1i          |- ( -. ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) -> ( $\psi$ -> ( -. $\phi$ -> $\chi$ ) ) ) 

com3r         |- ( -. $\phi$ -> ( -. ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) -> ( $\psi$ -> $\chi$ ) ) ) 

ax-1                 |- ( $\psi$ -> ( -. $\phi$ -> $\psi$ ) ) 

pm2.24               |- ( $\phi$ -> ( -. $\phi$ -> $\psi$ ) ) 

pm5.21ni            |- ( -. ( -. $\phi$ -> $\psi$ ) -> ( $\psi$ <-> $\phi$ ) ) 

con1i              |- ( -. ( $\psi$ <-> $\phi$ ) -> ( -. $\phi$ -> $\psi$ ) ) 

a1d               |- ( -. ( $\psi$ <-> $\phi$ ) -> ( $\chi$ -> ( -. $\phi$ -> $\psi$ ) ) ) 

con1i            |- ( -. ( $\chi$ -> ( -. $\phi$ -> $\psi$ ) ) -> ( $\psi$ <-> $\phi$ ) ) 

simplim          |- ( -. ( $\chi$ -> ( -. $\phi$ -> $\psi$ ) ) -> $\chi$ ) 

2thd            |- ( -. ( $\chi$ -> ( -. $\phi$ -> $\psi$ ) ) -> ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) ) 

con1i          |- ( -. ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) -> ( $\chi$ -> ( -. $\phi$ -> $\psi$ ) ) ) 

com3r         |- ( -. $\phi$ -> ( -. ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) -> ( $\chi$ -> $\psi$ ) ) ) 

impbidd      |- ( -. $\phi$ -> ( -. ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) -> ( $\psi$ <-> $\chi$ ) ) ) 

con1d       |- ( -. $\phi$ -> ( -. ( $\psi$ <-> $\chi$ ) -> ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) ) ) 

bija       |- ( ( $\phi$ <-> ( $\psi$ <-> $\chi$ ) ) -> ( ( $\psi$ <-> $\phi$ ) <-> $\chi$ ) ) 

syl5bb    |- ( ( $\phi$ <-> ( $\psi$ <-> $\chi$ ) ) -> ( ( $\phi$ <-> $\psi$ ) <-> $\chi$ ) ) 

impbii   |- ( ( ( $\phi$ <-> $\psi$ ) <-> $\chi$ ) <-> ( $\phi$ <-> ( $\psi$ <-> $\chi$ ) ) ) 
