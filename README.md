# Kaldanes
Kaldanes are a set of families of data structures and programming methods for a different kind of data management system.

    [Valverde Computing copyright notice]

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    As an exception, the copyright holders of this Library grant you permission to
    (i) compile an Application with the Library, and
    (ii) distribute the Application containing code generated by the Library and    
    added to the Application during this compilation process under terms of your choice,
    provided you also meet the terms and conditions of the Application license.

There are currently three C++ source code files and four C++ header files in this collection:

1. SortBench.cpp - a benchmark application for measuring the performance (clock time) of           
  various kinds of template sort methods (selection, shell, insertion, merge and quick sorts,           
  but not stl sort: I could not get past the const bug: "discards qualifiers" on two string types           
  when using simple arrays, and could not use the <array> type, because the program dynamically           
  changes array length and template numeric veriables must be const across the compile, I believe.)       
                  
  Different sort methods are used against different array element types (numeric, pointer strings,           
  Direct strings and Kaldane Symbiont strings (see below for descriptions of the last two.)           
  Various examples of how to operate the SortBench.cpp application are commented out in the main()           
  function. Multiple runs are sequentially executed. A catchall event handler is provided for           
  release (performance) testing, when you will need it if something goes wrong (and if it generates           
  an exception, most of the C++ runtime is the C runtime and does not generate exceptions on failure.)           
                  
  SortBench is currently set up to compare Direct and Kaldane Symbiont strings with std::string type           
  that you get from #include <string>. Kaldane Symbiont stings are across the board faster and sometimes           
  thousands of times so. std::string is quadratic for merge sort, Kaldane Symbionts are almost linear.           
           
2. SortDemo.cpp - a trimmed down program to demonstrate how much faster Direct strings are than the           
  std::string or <string> type for strings 64 bytes long and shorter, and to demonstrate how            
  significantly faster Kaldane Symbiont strings are than the std::string or <string> type ALL THE TIME.           
  The results from running SortDemo are prineted below.           
           
3. SortQATest.cpp - contains a test to drive the SortDemo-style run in a loop, checking for           
  boolean operational correctness on the new types and that the sorted output is actually sorted.           
  Throws exceptions if not.           
           
4. Direct.h - header file for Direct string type, which are variable-length (with an upper bound)           
  null-terminated byte strings that have their bytes moved as a single body when they are swapped           
  as in sorting, not just moving their pointers (following the Sort Benchmark rules.) These might           
  be a candidate for the Indy Sort. Direct has "value-string semantics" as Stroustrup defined for           
  his String type (C++11, chapter 19.3) Direct strings are the fastest for short strings: linear           
  for quick sort AND merge sort (as opposed to the type <string> which is slower for quick sort           
  and quadratic for merge sort.) However, even for longer strings Direct are sub-linear in time           
  complexity for string length due to cache pre-fetch (that means when you double the length of           
  the strings being sorted, the time taken is quite a bit less than double.) They are designed            
  to be used  with slab allocation/deallocation on the stack, as opposed to fine-grained allocators.           
  Since the allocated slab never needs to contain pointers, the slab data structure is base+offset           
  and can be mmapped to a file or /dev/shm and shared locally or across  a memory fabric like Gen-Z,           
  or stored and transmitted, or mmapped over an NFS: consistency considerations are an issue for           
  sharing, of course (caveat participem).           
           
5. Symbiont.h - header file for Kaldane Symbiont string, which are variable-length (with an upper           
  bound) null-terminated byte strings that have their indexes and a "poor man's normalized key"           
  (pmnk) moved when they are swapped, without moving their tail strings (ignoring the Sort Benchmark           
  rules). Symbiont has "value-string semantics" as Stroustrup defined for his String type (C++11,           
  chapter 19.3) These might be a candidate for the Indy Sort. Symbiont strings behave like Direct           
  strings for short lengths (32 bytes or less, by internally equating the pmnk length with the           
  string length.) This allows Symbiont strings to match Direct strings in performance for short           
  strings and still have constant time complexity for any string length: they are a candidate           
  for general purpose string programming with stack slab allocation/deallocation as opposed to           
  the fine-grained allocators, which are necessary for pointer strings like <string>. Symbiont           
  strings are quadratic in the debug build for  merge sort, and linear in the release build for           
  both quick sort and merge sort, so remember to use the release build for performance analysis.           
  They are designed to be used  with with slab allocation/deallocation on the stack, as opposed           
  to fine-grained allocators. Since the allocated slab never needs to contain pointers, only           
  array indices, the slab data structure is base+offset and can be mmapped to a file or /dev/shm           
  and shared locally or across a memory fabric such as Gen-Z, or stored and transmitted, or mmapped           
  over an NFS: consistency considerations are an issue for sharing, of course (caveat participem).           
           
6. Head.h - header file for Kaldane Head string, which are variable-length (with an upper           
  bound) null-terminated byte strings that have their indexes and a "poor man's normalized key"           
  (pmnk) moved when they are swapped, without moving their tail strings (ignoring the Sort Benchmark           
  rules). Head has "value-string semantics" as Stroustrup defined for his String type (C++11,           
  chapter 19.3) These might be a candidate for the Indy Sort. Head strings behave like Direct           
  strings for short lengths (16 bytes or less, by internally equating the pmnk length with the           
  string length.) This allows Head strings to match Direct strings in performance for short           
  strings and still have constant time complexity for any string length: they are a candidate           
  for general purpose string programming with stack slab allocation/deallocation as opposed to           
  the fine-grained allocators, which are necessary for pointer strings like std::string. Head           
  strings isolate the head (index + pmnk) from the tail, which deploys a Direct string with a           
  typedef called "tail". At scale, Head strings sort faster than Direct or Symbiont strings           
  and much, much faster than std::strings, which require much more space as well (probably due           
  to fine-grainied allocation.) The differences are smaller for short strings but become more           
  pronounced for longer strings and  especially for merge sorts, where std:string is quadratic           
  in the release build. Both merge and quick sorts are effectively linear for Head strings and           
  constant for string length. Head strings, like Symbiont strings are quadratic in the debug           
  build for  merge sort, so remember to use the release build for performance analysis.           
  They are designed to be used  with with slab allocation/deallocation on the stack, as opposed           
  to fine-grained allocators. Since the two allocated slabs never need to contain pointers, only           
  array indices, the slab data structures are base+offset and can be mmapped to a file or /dev/shm           
  and shared locally or across a memory fabric such as Gen-Z, or stored and transmitted, or mmapped           
  over an NFS: consistency considerations are an issue for sharing, of course (caveat participem).           
           
7. Sorts.h - header file containing various templated sort methods.           
           
  The name Kaldane is an antique SciFi reference (heads move, bodies stay):           
  http://www.catspawdynamics.com/images/gino_d%27achille_5-the_chessmen_of_mars.jpg           
  or google image search the word "kaldane" for other images           
  https://en.wikipedia.org/wiki/Kaldane           
           
There are also various performancce text data files from SortDemo and big runs from SortBench against the three           
data types: Direct, Symbiont and std::string.           
______________________________________________________________________________________

The results of running SortDemo on my Centos 7 system are below.           
           
It's a ~$2nn refurbished 15GB HP 6300 Pro Small Format box (from Amazon.com), compiling with commands:           
g++ -Wall -fexceptions -O2 -std=c++11  -c /home/charles/Projects/SortBench/SortDemo.cpp -o obj/Release/SortDemo.o           
g++  -o bin/Release/SortBench obj/Release/SortDemo.o  -s             
The compiler version is "gcc version 4.8.5 20150623 (Red Hat 4.8.5-16) (GCC)"           
           
Here are the results of the "lscpu" command on my system:           
           
[charles@localhost ~]$ lscpu           
Architecture:          x86_64           
CPU op-mode(s):        32-bit, 64-bit           
Byte Order:            Little Endian           
CPU(s):                4           
On-line CPU(s) list:   0-3           
Thread(s) per core:    1           
Core(s) per socket:    4           
Socket(s):             1           
NUMA node(s):          1           
Vendor ID:             GenuineIntel           
CPU family:            6           
Model:                 58           
Model name:            Intel(R) Core(TM) i5-3570 CPU @ 3.40GHz           
Stepping:              9           
CPU MHz:               1989.132           
CPU max MHz:           3800.0000           
CPU min MHz:           1600.0000           
BogoMIPS:              6784.92           
Virtualization:        VT-x           
L1d cache:             32K           
L1i cache:             32K           
L2 cache:              256K           
L3 cache:              6144K           
NUMA node0 CPU(s):     0-3           
Flags:                 fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm epb tpr_shadow vnmi flexpriority ept vpid fsgsbase smep erms xsaveopt dtherm ida arat pln pts
[charles@localhost ~]$ 

______________________________________________________________________________________

SortDemo output (the three merge sorts for the std::string type take less than
30 minutes each, because it is quadratic on std:string):
______________________________________________________________________________________

Three different string types, identical in content and           
sorting logic, but not in timings.           
           
Note that the number of swaps and compares for identical arrays,           
   using the identical code: these statistics should be identical.           
           
Remember to set the C++11 switch in the IDE or compiler!           
           
Remember to use release builds if you are analyzing performance,           
otherwise Symbionts will be very slow!           
           
Remember to set the "ulimit -s" soft and hard stack limits to unlimited,           
otherwise it can die!]           
           
           
Quick Sort 100 10-byte strings           
Direct: 14.645 microseconds, 268.000 swaps, 1022.000 compares           
Symbiont: 11.897 microseconds, 268.000 swaps, 1022.000 compares           
Head: 11.057 microseconds, 268.000 swaps, 1022.000 compares           
std::string: 18.761 microseconds, 268.000 swaps, 1022.000 compares           
           
Merge Sort 100 10-byte strings           
Direct: 13.572 microseconds, 622.000 swaps, 549.000 compares           
Symbiont: 11.346 microseconds, 622.000 swaps, 549.000 compares           
Head: 10.811 microseconds, 622.000 swaps, 549.000 compares           
std::string: 31.152 microseconds, 622.000 swaps, 549.000 compares           
           
Quick Sort 100 100-byte strings           
Direct: 20.720 microseconds, 255.000 swaps, 885.000 compares           
Symbiont: 12.674 microseconds, 255.000 swaps, 885.000 compares           
Head: 11.362 microseconds, 255.000 swaps, 885.000 compares           
std::string: 20.136 microseconds, 255.000 swaps, 885.000 compares           
           
Merge Sort 100 100-byte strings           
Direct: 22.075 microseconds, 627.000 swaps, 541.000 compares           
Symbiont: 12.063 microseconds, 627.000 swaps, 541.000 compares           
Head: 11.518 microseconds, 627.000 swaps, 541.000 compares           
std::string: 29.883 microseconds, 627.000 swaps, 541.000 compares           
           
Quick Sort 100 1000-byte strings           
Direct: 64.651 microseconds, 274.000 swaps, 879.000 compares           
Symbiont: 11.594 microseconds, 274.000 swaps, 879.000 compares           
Head: 13.465 microseconds, 274.000 swaps, 879.000 compares           
std::string: 27.880 microseconds, 274.000 swaps, 879.000 compares           
           
Merge Sort 100 1000-byte strings           
Direct: 109.945 microseconds, 632.000 swaps, 542.000 compares           
Symbiont: 11.282 microseconds, 632.000 swaps, 542.000 compares           
Head: 11.123 microseconds, 632.000 swaps, 542.000 compares           
std::string: 30.449 microseconds, 632.000 swaps, 542.000 compares           
           
Quick Sort 10000 10-byte strings           
Direct: 2068.391 microseconds, 42517.000 swaps, 174901.000 compares           
Symbiont: 2011.203 microseconds, 42517.000 swaps, 174901.000 compares           
Head: 1607.907 microseconds, 42517.000 swaps, 174901.000 compares           
std::string: 3202.705 microseconds, 42517.000 swaps, 174901.000 compares           
           
Merge Sort 10000 10-byte strings           
Direct: 2260.241 microseconds, 128141.000 swaps, 120520.000 compares           
Symbiont: 2061.154 microseconds, 128141.000 swaps, 120520.000 compares           
Head: 1796.691 microseconds, 128141.000 swaps, 120520.000 compares           
std::string: 87963.226 microseconds, 128141.000 swaps, 120520.000 compares           
           
Quick Sort 10000 100-byte strings           
Direct: 3010.562 microseconds, 42485.000 swaps, 180484.000 compares           
Symbiont: 1987.120 microseconds, 42485.000 swaps, 180484.000 compares           
Head: 1956.443 microseconds, 42485.000 swaps, 180484.000 compares           
std::string: 2991.259 microseconds, 42485.000 swaps, 180484.000 compares           
           
Merge Sort 10000 100-byte strings           
Direct: 3468.323 microseconds, 128131.000 swaps, 120557.000 compares           
Symbiont: 2298.974 microseconds, 128131.000 swaps, 120557.000 compares           
Head: 1822.172 microseconds, 128131.000 swaps, 120557.000 compares           
std::string: 86915.250 microseconds, 128131.000 swaps, 120557.000 compares           
           
Quick Sort 10000 1000-byte strings           
Direct: 10526.277 microseconds, 41953.000 swaps, 194134.000 compares           
Symbiont: 2178.344 microseconds, 41953.000 swaps, 194134.000 compares           
Head: 2186.444 microseconds, 41953.000 swaps, 194134.000 compares           
std::string: 5068.229 microseconds, 41953.000 swaps, 194134.000 compares           
           
Merge Sort 10000 1000-byte strings           
Direct: 25063.884 microseconds, 128158.000 swaps, 120402.000 compares           
Symbiont: 3103.403 microseconds, 128158.000 swaps, 120402.000 compares           
Head: 1816.093 microseconds, 128158.000 swaps, 120402.000 compares           
std::string: 87058.274 microseconds, 128158.000 swaps, 120402.000 compares           
           
Quick Sort 1000000 10-byte strings           
Direct: 283195.074 microseconds, 5757874.000 swaps, 28292342.000 compares           
Symbiont: 295052.308 microseconds, 5757874.000 swaps, 28292342.000 compares           
Head: 229400.401 microseconds, 5757874.000 swaps, 28292342.000 compares           
std::string: 674745.971 microseconds, 5757874.000 swaps, 28292342.000 compares           
           
           
warning: the std::string merge sorts of 1 million records can take 6740 times longer!           
(30 minutes or so on my i5-3570 CPU)           
           
           
Merge Sort 1000000 10-byte strings           
Direct: 336163.630 microseconds, 19344209.000 swaps, 18673922.000 compares           
Symbiont: 285323.926 microseconds, 19344209.000 swaps, 18673922.000 compares           
Head: 252355.182 microseconds, 19344209.000 swaps, 18673922.000 compares           
std::string: 1679176824.622 microseconds, 19344209.000 swaps, 18673922.000 compares           
           
Quick Sort 1000000 100-byte strings           
Direct: 516994.376 microseconds, 5777478.000 swaps, 27405818.000 compares           
Symbiont: 439473.179 microseconds, 5777478.000 swaps, 27405818.000 compares           
Head: 257235.596 microseconds, 5777478.000 swaps, 27405818.000 compares           
std::string: 703670.424 microseconds, 5777478.000 swaps, 27405818.000 compares           
           
Merge Sort 1000000 100-byte strings           
Direct: 708296.031 microseconds, 19346390.000 swaps, 18673361.000 compares           
Symbiont: 585316.765 microseconds, 19346390.000 swaps, 18673361.000 compares           
Head: 276008.034 microseconds, 19346390.000 swaps, 18673361.000 compares           
std::string: 1672464026.004 microseconds, 19346390.000 swaps, 18673361.000 compares           
           
Quick Sort 1000000 1000-byte strings           
Direct: 1825963.622 microseconds, 5820251.000 swaps, 26417245.000 compares           
Symbiont: 438633.454 microseconds, 5820251.000 swaps, 26417245.000 compares           
Head: 255052.300 microseconds, 5820251.000 swaps, 26417245.000 compares           
std::string: 991330.925 microseconds, 5820251.000 swaps, 26417245.000 compares           
           
Merge Sort 1000000 1000-byte strings           
Direct: 5289084.353 microseconds, 19344894.000 swaps, 18675438.000 compares           
Symbiont: 655764.231 microseconds, 19344894.000 swaps, 18675438.000 compares           
Head: 274903.353 microseconds, 19344894.000 swaps, 18675438.000 compares           
std::string: 1667379999.981 microseconds, 19344894.000 swaps, 18675438.000 compares           
           
Process returned 0 (0x0)   execution time : 5052.820 s           
Press ENTER to continue.           
                                                    
______________________________________________________________________________________

