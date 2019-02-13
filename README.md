# Kaldanes
Kaldanes are a set of families of data structures and programming methods for a different kind of relational database system.

To build on Linux, you must have a c++11 compiler 4.8.5 or later:

	make clean
	make all

To run programs do this command to allow the stack to grow:

	ulimit -s unlimited

To look at the "Technical White Paper" explaining all the gory details, or a tutorial on "Distributed Resilience & High Availability" it is better to download them (the github viewer cuts them short and the URLs and navigation links don't connect), first click below, then click the "download" button to get the pdf file:

https://github.com/charlesone/Kaldanes/blob/master/Technical%20White%20Paper.pdf

or

https://github.com/charlesone/Kaldanes/blob/master/Tutorial%20-%20Distributed%20Resilience%20%26%20High%20Availability.pdf

Here is a discussion on stackoverflow.com concerning just one of the reasons behind this work:

https://stackoverflow.com/questions/54562770/performance-of-big-slabs-due-to-allocation-initialization-and-deallocation-over

Here is a discussion on Chris Date and Hugh Darwen's Third Manifesto Forum discussing this work:

https://forum.thethirdmanifesto.com/forum/topic/kaldanes-and-high-speed-joins/

See the wiki for more information:
https://github.com/charlesone/Kaldanes/wiki

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

There are currently six C++ source code files and eight C++ header files in this collection:

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

  SortBench is currently set up to compare three Kaldanes types: Direct, Symbiont and Head strings with
  std::string that you get from #include <string>. Kaldane Head strings are across the board fastest and
  sometimes thousands of times faster than std::string. std::string is quadratic for merge sort,
  whereas Head strings are linear. Direct strings are the most efficient for slab allocation.

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

8. RowString.h - header file for RowString class (variation of Direct) in simple arrays for tables

  Both the RowString template class and its accessing IndexString template class share the same 5
  basic template parameters and operate on character <typename T> strings, although the current
  implementation is unabashedly char typed and won’t work for other string types (yet.) Both classes
  are completely base + offset, accessible under virtual addressing, and the objects are portable
  individually as elements or as slabs: for ease of replication. The variable format is the log record
  format. The current demo code base does not use mmap for accessing these objects, but that will work
  just fine in future.

  RowString variables are variable-length (with an upper bound) collections of null-terminated byte
  strings that have their bytes moved as a single body when they are swapped as in sorting, not just
  moving their pointers (following the Sort Benchmark rules.) RowString has "value-string semantics"
  as Stroustrup defined for his String type (The C++11 Programming Language, 4th edition, chapter 19.3).

9. IndexString.h - header file for IndexString class (variation of Head) in simple arrays for
  indexes on its RowString friend class

  As stated before, the IndexString class starts with the identical template parameters as the
  RowString class table that it creates an index for. In addition, IndexString has a separate
  tuning optional template parameter named pmnkSize, which defaults to 7, the sweet spot for
  well-behaved strings. Both IndexString and RowString carry their internals in a struct, because
  that makes copying and sizing operations more convenient, and the sizes of those are in
  increments of 4 bytes from C++.

  Poor Man’s Normalized Keys (pmnk)

  The IndexString struct contains the 4-byte K-value as an index into the array of IndexString
  objects at the front, and then the char pmnk[pmnkSize + 1] null terminated string that is the
  head, of all of the column strings inside the RowString that are being indexed. That means that
  pmnkSize should logically have values of 3, 7, 11, 15… for strings without lots of repeating
  characters in the front (e.g., “           Joseph          Cotton“) and that have a somewhat
  random distribution, 7 is a good average size and that keeps the system from having to jump to
  the rest of the string kept in the RowString column, because the pmnk kept in the IndexString
  was too short to satisfy some comparison.

  As is stated in Goetz Graefe’s paper: B-tree indexes and CPU Caches, Goetz Graefe and Per-Ake
  Larson: IEEE Proceedings of the 17th International Conference on Data Engineering Section 6.1,
  "Poor Man's Normalized Keys", p. 355:

	"This technique works well only if common key prefixes are truncated before the poor man's
    normalized keys are extracted. In that case, a small poor man's normalized key (e.g., 2 bytes)
    may be sufficient to avoid most cache faults on the full records; if prefix truncation is not
    used, even a fairly large poor man's normalized key may not be very effective, in particular
    in B-tree leaves."

  It might be cautiously suggested that indexes maintained on data requiring a lot of prefix
  truncation might have lower value in any case: mining low-grade ore is bound to be less
  satisfying than mining high-grade ore.

10. RelationVector.h - header file for a database relation using indexes with a directional component
  for automatic optimization. RelationVector class holds a from column and a to column as
  IndexString class objects.

  In an SQL language query that joins tables, the connections between tables are reflected in a
  where clause or a join statement with foreign keys such as:

	SELECT table1.column1, table2.column4, table1.column3
	FROM table1
	WHERE table1.column3 = table2.column7;
	
  Then the offline tools, in particular the SQL compiler and optimizer, make informed choices as
  to what that statement means at runtime. For more complex joins, there might be no connection
  between the parts of the join: for example, two sets of where clauses of four completely
  separate tables will have no linkage for the compiler to generate a query from: that will
  generate an SQL error, unless there is some rule to rectify it.

  In the relational database system presented in the current code base, it was decided to use
  RelationVector items as template parameters, which contain (from-column, to-column) IndexString
  pairs on one or two tables with a vectored direction, and the order of those relation vectors in
  a set to deduce the join method for that query.

11. JoinedRow.h - header file for two classes: QueryPlan and JoinedRow

  QueryPlan Class

  A heavyweight relational QueryPlan class is defined by a variadic template parameter pack of
  ordered RelationVector class objects and creates a shared tuple of them

  The strategy for the QueryPlan class was that it would bear the burden of future metadata and
  join query initialization and processing required by the extremely lightweight JoinedRow class.
  In the current code base that burden is middling, but that will grow as more features are
  added. Of course that meant it had to precede the JoinedRow class in the declaration order, and
  yet be deeply coupled to it by a lot of machinery. So, both classes are located in the same
  header JoinedRow.h in the obvious order and share static globals for the handovers back and
  forth.

  JoinedRow Class

  A lightweight relational JoinedRow class is defined by the identical template class parameters
  as its friend class QueryPlan and is coupled to that plan at runtime

  JoinedRow class objects are microscopic (a 4-byte int per table participating in the join)
  compared to the table and index data that they reference directly, and the indirect access to
  all the metadata on all those tables and every other column and index on those columns in those
  tables.

12. TableDemo.cpp - 

  As features were added to this nascent relational database system, the TableDemo program
  exposes that history and it still serves as an eyeball check to see if the latest changes have
  added bugs breaking earlier coding.

  The OpenFlights.org Air Routes Database

  The database used was accessed from the database used by OpenFlights.org. The files contain
  data on all the air routes, airports, and airlines as of 2009, not maintained since. At that
  time there were 65,612 valid air routes (with valid source and destination airports, and
  airlines.) It’s a small database, only 3.5 MB, but you can do big joins on a small database.
  In reference to the relatively small size of database used in the demo programs, the argument
  for whether the current code base will scale to enormous size, has three parts: (1) the index
  sorts are linear time complexity relative to the count of rows while using a small PMNK index
  element, (2) the nested loop joins are linear time complexity relative to the number of output
  joined rows, and also linear relative to the number of range queries, which have an average
  2(log2(n)-1) time complexity for consistent databases, where n is the number of table rows,
  and (3) the memoized join time complexity is effectively constant time. If you think of the
  compile cost as including the memoized join cost, then compiling the relational database
  queries only occurs one time per generation of the database.

  At OpenFlights.org, the data is stored in .dat files, those were converted to .csv files by
  removing quoted strings and replacing commas in those quoted strings with semicolons to
  satisfy the limited .csv input functioning of the current code base. Everything is #-commented
  (R .csv style) in the resulting .csv files presented in the code base.

  The relations between the demo program tables are (1) many airlines serve many airports [m:m],
  (2) routes are flown by one airline + airlines serve many routes [1:m], and (3 and 4) routes
  have one source and destination airport + airports have many source routes [1:m] and many
  destination routes [1:m].

  Once again, the TableDemo program, as all the demo programs do, has two sections before the
  main() function: (1) a Static Metadata Section (SMS) laid out in an absolutely order critical
  way containing everything (classes, types, static data, const and constexpr) that is needed
  for generic programming (compile-time) of the relational database system, and (2) an Active
  Program Section (APS) laid out in whatever order that is desired, in this case containing
  performance analysis declarations and functions, data loading and printing functions, and a
  runtime checker to partially guarantee the consistency of columns versus tables in the layout
  in the SMS.

13. JoinDemo.cpp - Exploring joins and memoized joins on tables with indexes using Kaldanes

  JoinDemo keeps track of the time consumed by operations and printing those operations
  separately, using the nanosecond clock facility of C++11 on Linux. Initially it builds the
  database:

  1.    The three tables are allocated and loaded from .csv files and then anchored into RowString
        arrays.
  2.    Space for the ten indexes on those three tables is allocated.
  3.    The table source anchor is copied into the indexes, the index keys are copied into the
        indexes from the tables, and the indexes are sorted.

  At this point the database is up and ready to query: for the three relational database system
  demos (TableDemo, JoinDemo and BigJoinDemo,) the average time to produce the database and
  bring it up from scratch using input .csv files was 115.556 ms, less than 1/8th of a second.

  JoinDemo executes query logic on the scale of the entire size of the database. The relations
  are slightly different, but it uses the identical schema presented in the picture above, simply
  going at those 65,612 valid air routes from the access path of airlines instead of airports.
  The query for JoinDemo identifies and counts the valid routes for the top ten air carriers in
  the world.

14. BigJoinDemo.cpp - Exploring big joins and memoized joins on tables with indexes using Kaldanes

  BigJoinDemo keeps track of the time consumed by operations and printing those operations
  separately, using the nanosecond clock facility of C++11 on Linux. Initially it builds the
  database:

  1.    The three tables are allocated and loaded from .csv files and then anchored into RowString
        arrays.
  2.    Space for the ten indexes on those three tables is allocated.
  3.    The table source anchor is copied into the indexes, the index keys are copied into the
        indexes from the tables, and the indexes are sorted.

  At this point the database is up and ready to query: for the three relational database system
  demos (TableDemo, JoinDemo and BigJoinDemo,) the average time to produce the database and
  bring it up from scratch using input .csv files was 115.556 ms, less than 1/8th of a second.

  Since there are only 65,612 valid air routes in this database, simple queries on valid routes
  will not allow bigger results to test the performance at scale. However, what if the question
  is asked of the database, “How many double routes would there be if at the end of any flight
  on one of those 65,612 valid routes, the passenger hopped on any plane from any airline
  leaving that destination airport to go anywhere you could fly?”

  Those could called “double routes” where the destination airport of that route was linked as
  the source airport of any other route for any airline. This might be an interesting query for
  disease transmission, tracking criminal activity or missing persons, etc.

  The name Kaldane is an antique SciFi reference (heads move, bodies stay):
  http://www.catspawdynamics.com/images/gino_d%27achille_5-the_chessmen_of_mars.jpg
  or google image search the word "kaldane" for other images
  https://en.wikipedia.org/wiki/Kaldane
______________________________________________________________________________________

Once again, to look at the "Technical White Paper" explaining all the gory details, or a tutorial on "Distributed Resilience & High Availability" it is better to download them (the github viewer cuts them short and the URLs and navigation links don't connect), first click below, then click the "download" button to get the pdf file:

https://github.com/charlesone/Kaldanes/blob/master/Technical%20White%20Paper.pdf

or

https://github.com/charlesone/Kaldanes/blob/master/Tutorial%20-%20Distributed%20Resilience%20%26%20High%20Availability.pdf

See the wiki for more information:
https://github.com/charlesone/Kaldanes/wiki

______________________________________________________________________________________

