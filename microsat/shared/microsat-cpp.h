#pragma once

/*****************************************************************[microsat-cpp.h]***

  The MIT License

  Copyright (c) 2014-2018 Marijn Heule, C++ port by Stephan Brumme in 2020

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*************************************************************************************

  This a C++ port of Marijn Heule's MicroSAT code (see https://github.com/marijnheule/microsat )

  code example:
    MicroSAT s(2);                                                 // set number of variables
    s.add(-2);                                                     // add a unit
    auto clause = { -1, +2 }; s.add(clause);                       // add a clause
    if (s.solve()) std::cout <<   "SATISFIABLE" << std::endl;      // run solver and print result
    else           std::cout << "UNSATISFIABLE" << std::endl;
    std::cout << "variable 1 is " << std::boolalpha << s.query(1); // query variable (true or false)

  If you know the DIMACS CNF file format then you already know the syntax. For everybody else:
  - units:
    * a positive number x refers to variable x which should be true
    * a negative number y refers to variable y which should be false
  - clauses:
    * consist of multiple literals
    * a positive number x refers to the         value of variable x
    * a negative number y refers to the negated value of variable y
  - zero is a special marker denoting the end of a clause and should not be used for the add() function.

  In the example above, two variables x1 and x2 exist:
  - the constructor needs to know the total number of variables (see "MicroSAT s(2);")
  - x2 is defined as false (see "s.add(-2);")
  - x1 is initially unknown (there's no "s.add(+1);" or "s.add(-1);")
  - one clause / constraint needs to be fulfilled:
    ((not x1) or x2) must be true
  - "s.solve()" will return true, thus "SATISFIABLE" is printed
  - x1 is now resolved and "s.query(1);" evaluates to false
    (since the contraint reduces to ((not x1) or false) == (not x1)
     which is only satisfiable if x1 is false)

  Often useful: "not (a and b)" is the same as "(not a) or (not b)" (De Morgan's laws)
  you can't express the former as a MicroSAT clause but the latter is simply:
  auto clause = { -a, -b }; s.add(clause);

  If solve() returns true then you may want to get a detailled solution, too:
  the function query(x) returns whether variable x is true or false in the solution found

  A const char* exception is thrown if default memory allocation wasn't sufficient: "out of memory"
  Just increase the constructor's second parameter (default: 1 << 20, which means 1 million temporaries).
*************************************************************************************/

class MicroSAT {
protected:
  int   m_nVars;                                            // The variables are described in the initCDCL procedure
  int  *m_DB, *m_buffer, *m_reason, *m_falseStack, *m_first, *m_forced, *m_processed, *m_assigned, *m_next, *m_prev;
  unsigned int m_mem_used, m_mem_max, m_mem_fixed, m_maxLemmas, m_nLemmas, m_fast, m_slow, m_head;
  char *m_false; bool *m_model;

  enum { END = -9, UNSAT = 0, SAT = 1, MARK = 2, NOTIMPLIED = 5, IMPLIED = 6 };// Internal constants, DON'T CHANGE
  enum { InitialMaxLemmas = 2000, LemmaIncrement = 300 };   // Adjust these two lemma constants to tweak performance

  inline int abs (int x) { return x >= 0 ? +x : -x; }       // Return absolute value (avoids #include <cstdlib> )

  template <typename T>
  T* getMemory (unsigned int numElements) {                 // Allocate memory for mem_size integers
    const unsigned int PerInt = sizeof (int) / sizeof (T);  // Compact storage for non-ints
    unsigned int mem_size = numElements;
    if (PerInt > 1) mem_size = (numElements + PerInt - 1) / PerInt;
    if (m_mem_used + mem_size > m_mem_max)                  // Check whether still some space available
      throw "out of memory";
    int* store = m_DB + m_mem_used;                         // Compute a pointer to the new memory location
    m_mem_used += mem_size;                                 // Update the size of the used memory
    return (T*) store; }                                    // Return the pointer

  void assign (const int* reason, bool forced) {            // Make the first literal of the reason true
    int lit = reason[0];                                    // Let lit be the first literal in the reason
    m_false[-lit] = forced ? IMPLIED : SAT;                 // Mark lit as true and IMPLIED if forced
    *(m_assigned++) = -lit;                                 // Push it on the assignment stack
    m_reason[abs (lit)] = 1 + (int) (reason - m_DB);        // Set the reason clause of lit
    m_model [abs (lit)] = (lit > 0); }                      // Mark the literal as true in the model

  inline void unassign (int lit) { m_false[lit] = UNSAT; }  // Unassign the literal

  inline void addWatch (int lit, unsigned int mem) {        // Add a watch pointer to a clause containing lit
    m_DB[mem] = m_first[lit]; m_first[lit] = mem; }         // By updating the database and the pointers

  const int* addClause (const int* in, unsigned int size, bool irr) { // Adds a clause stored in *in of size size
    unsigned int i, used = m_mem_used;                      // Store a pointer to the beginning of the clause
    int* clause = getMemory<int> (size + 3) + 2;            // Allocate memory for the clause in the database
    if (size > 1) { addWatch (in[0], used  );               // If the clause is not unit, then add
                    addWatch (in[1], used+1); }             // Two watch pointers to the datastructure
    for (i = 0; i < size; i++) clause[i] = in[i];           // Copy the clause from the buffer to the database
    clause[i] = 0;
    if (irr) m_mem_fixed = m_mem_used; else m_nLemmas++;    // Update the statistics
    return clause; }                                        // Return the pointer to the clause in the database

  void restart () {                                         // Perform a restart (i.e., unassign all variables)
    while (m_assigned > m_forced) unassign (*(--m_assigned)); // Remove all unforced false lits from falseStack
    m_processed = m_forced; }                               // Reset the processed pointer

  void reduceDB (unsigned int keep = 6) {                   // Removes "less useful" lemmas from DB, keep 6 lemmas
    while (m_nLemmas > m_maxLemmas) m_maxLemmas += LemmaIncrement; // Allow more lemmas in the future
    m_nLemmas = 0;                                          // Reset the number of lemmas
    for (int i = -m_nVars; i <= +m_nVars; i++) {            // Loop over the variables
      if (i == 0) continue;                                 // Variable 0 remains unused
      int* watch = &m_first[i];                             // Get the pointer to the first watched clause
      while (*watch != END)                                 // As long as there are watched clauses
        if (*watch < (int) m_mem_fixed) watch = &m_DB[*watch];   // Remove the watch if it points to a lemma
        else                           *watch =  m_DB[*watch]; } // Otherwise (meaning an input clause) go to next watch
    int old_used = m_mem_used; m_mem_used = m_mem_fixed;    // Virtually remove all lemmas
    for (int i = m_mem_fixed + 2; i < old_used; i += 3) {   // While the old memory contains lemmas
      unsigned int count = 0, head = i;                     // Get the lemma to which the head is pointing
      while (m_DB[i]) { int lit = m_DB[i++];                // Count the number of literals
        if ((lit > 0) == m_model[abs (lit)]) count++; }     // That are satisfied by the current model
      if (count < keep) addClause (m_DB+head, i-head, false); } } // If the latter is smaller than k, add it back

  void bump (int lit) {                                     // Move the variable to the front of the decision list
    if (m_false[lit] == IMPLIED) return;                    // Nothing to do if implied
    m_false[lit] = MARK;                                    // MARK the literal as involved if not a top-level unit
    unsigned int var = abs (lit); if (var == m_head) return;// In case var is not already the head of the list
    m_prev[m_next[var]] = m_prev[var];                      // Update the prev link, and
    m_next[m_prev[var]] = m_next[var];                      // Update the next link, and
    m_next[m_head] = var;                                   // Add a next link to the head, and
    m_prev[var] = m_head; m_head = var;  }                  // Make var the new head

  bool implied (int lit) {                                  // Check if lit(eral) is implied by MARK literals
    if (m_false[lit] > MARK)  return (m_false[lit] & MARK); // If checked before return old result
    if (!m_reason[abs (lit)]) return false;                 // In case lit is a decision, it is not implied
    const int* p = &m_DB[m_reason[abs (lit)] - 1];          // Get the reason of lit(eral)
    while (*(++p))                                          // While there are literals in the reason
      if ((m_false[*p] ^ MARK) && !implied (*p)) {          // Recursively check if non-MARK literals are implied
        m_false[lit] = NOTIMPLIED; return false; }          // Mark and return not implied (denoted by IMPLIED - 1)
    m_false[lit] = IMPLIED; return true; }                  // Mark and return that the literal is implied

  const int* analyze (const int* clause) {                  // Compute a resolvent from falsified clause
    while (*clause) bump (*(clause++));                     // MARK all literals in the falsified clause
    while (m_reason[abs (*(--m_assigned))]) {               // Loop on variables on falseStack until the last decision
      if (m_false[*m_assigned] == MARK) {                   // If the tail of the stack is MARK
        const int* check = m_assigned;                      // Pointer to check if first-UIP is reached
        while (m_false[*(--check)] != MARK)                 // Check for a MARK literal before decision
          if (!m_reason[abs (*check)]) goto build;          // Otherwise it is the first-UIP so break
        clause = &m_DB[m_reason[abs (*m_assigned)]];        // Get the reason and ignore first literal
        while (*clause) bump (*(clause++)); }               // MARK all literals in reason
      unassign (*m_assigned); }                             // Unassign the tail of the stack

    build:; unsigned int size = 0, lbd = 0, flag = 0;       // Build conflict clause; Empty the clause buffer
    int* p = m_processed = m_assigned;                      // Loop from tail to front
    while (p >= m_forced) {                                 // Only literals on the stack can be MARKed
      if ((m_false[*p] == MARK) && !implied (*p)) {         // If MARKed and not implied
        m_buffer[size++] = *p; flag = 1; }                  // Add literal to conflict clause buffer
      if (!m_reason[abs (*p)]) { lbd += flag; flag = 0;     // Increase LBD for a decision with a true flag
        if (size == 1) m_processed = p; }                   // And update the processed pointer
      m_false[*(p--)] = SAT; }                              // Reset the MARK flag for all variables on the stack

    m_fast -= m_fast >>  5; m_fast += lbd << 15;            // Update the fast moving average
    m_slow -= m_slow >> 15; m_slow += lbd <<  5;            // Update the slow moving average

    while (m_assigned > m_processed)                        // Loop over all unprocessed literals
      unassign (*(m_assigned--));                           // Unassign all lits between tail & head
    unassign (*m_assigned);                                 // Assigned now equal to processed
    m_buffer[size] = 0;                                     // Terminate the buffer (and potentially print clause)
    return addClause (m_buffer, size, false); }             // Add new conflict clause to redundant DB

  bool propagate () {                                       // Performs unit propagation
    bool forced = m_reason[abs (*m_processed)] != 0;        // Initialize forced flag
    while (m_processed < m_assigned) {                      // While unprocessed false literals
      int lit = *(m_processed++);                           // Get first unprocessed literal
      int* watch = &m_first[lit];                           // Obtain the first watch pointer
      while (*watch != END) {                               // While there are watched clauses (watched by lit)
        bool unit = true;                                   // Let's assume that the clause is unit
        int* clause = &m_DB[*watch + 1];                    // Get the clause from DB
        if (clause[-2] ==   0) clause++;                    // Set the pointer to the first literal in the clause
        if (clause[ 0] == lit) clause[0] = clause[1];       // Ensure that the other watched literal is in front
        for (int i = 2; unit && clause[i]; i++)             // Scan the non-watched literals
          if (!m_false[clause[i]]) {                        // When clause[i] is not false, it is either true or unset
            clause[1] = clause[i]; clause[i] = lit;         // Swap literals
            int store = *watch; unit = false;               // Store the old watch
            *watch = m_DB[*watch];                          // Remove the watch from the list of lit
            addWatch (clause[1], store); }                  // Add the watch to the list of clause[1]
        if (unit) {                                         // If the clause is indeed unit
          clause[1] = lit; watch = &m_DB[*watch];           // Place lit at clause[1] and update next watch
          if ( m_false[-clause[0]]) continue;               // If the other watched literal is satisfied continue
          if (!m_false[ clause[0]])                         // If the other watched literal is falsified,
            assign (clause, forced);                        // A unit clause is found, and the reason is set
          else { if (forced) return false;                  // Found a root level conflict -> UNSAT
            const int* lemma = analyze (clause);            // Analyze the conflict return a conflict clause
            if (!lemma[1]) forced = true;                   // In case a unit clause is found, set forced flag
            assign (lemma, forced); break; } } } }          // Assign the conflict clause as a unit
    if (forced) m_forced = m_processed;                     // Set m_forced if applicable
    return true; }                                          // Finally, no conflict was found

  void init (unsigned int nVars, unsigned int mem_max) {    // Same parameters as constructor
    m_nVars = nVars; if (m_nVars == 0) m_nVars = 1;         // The code assumes that there is at least one variable
    m_model = new bool[m_nVars + 1];                        // Allocate memory for the final variable assignment
    m_mem_max = mem_max >= 10*nVars ? mem_max : 10*nVars;   // Need at least about 10 temporary integers per variable
    m_DB = new int[m_mem_max];                              // Allocate the initial database

    m_mem_used      = 0;                                    // The number of integers allocated in the DB
    m_nLemmas       = 0;                                    // The number of learned clauses -- redundant means learned
    m_maxLemmas     = InitialMaxLemmas;                     // Initial maximum number of learned clauses (default: 2000)
    m_fast = m_slow = 1 << 24;                              // Initialize the fast and slow moving averages

    m_buffer     = getMemory<int>  (m_nVars  );             // A buffer to store a temporary clause
    m_next       = getMemory<int>  (m_nVars+1);             // Next     variable in the heuristic order
    m_prev       = getMemory<int>  (m_nVars+1);             // Previous variable in the heuristic order
    m_reason     = getMemory<int>  (m_nVars+1);             // Array of clauses
    m_falseStack = getMemory<int>  (m_nVars+1);             // Stack of falsified literals -- this pointer is never changed
    m_forced     = m_falseStack;                            // Points inside *falseStack at first decision (unforced literal)
    m_processed  = m_falseStack;                            // Points inside *falseStack at first unprocessed literal
    m_assigned   = m_falseStack;                            // Points inside *falseStack at last  unprocessed literal
    m_false      = getMemory<char> (2*m_nVars+1) + m_nVars; // Labels for variables, non-zero means false
    m_first      = getMemory<int>  (2*m_nVars+1) + m_nVars; // Offset of the first watched clause
    m_DB[m_mem_used++] = 0;                                 // Make sure there is a 0 before the clauses are loaded

    for (int i = 1; i <= m_nVars; i++) {                    // Initialize the main datastructures:
      m_prev [i] = i - 1; m_next[i-1] = i;                  // Double-linked list for variable-move-to-front,
      m_model[i] = false;                                   // Model (phase-saving)
      m_false[i] = m_false[-i] = UNSAT;                     // The false array,
      m_first[i] = m_first[-i] = END; }                     // And first (watch pointers)
    m_false[0] = 0;                                         // Stop-marker
    m_head = m_nVars; }                                     // Initialize the head of the double-linked list

/*************************** public interface **************************************/
public:
  MicroSAT (unsigned int nVars, unsigned int mem_max = 1 << 20) { // 2^20 ints => about a million temporaries
    init (nVars, mem_max); }                                // Prepare data structures

  virtual ~MicroSAT () { delete[] m_model; delete[] m_DB; } // Deallocate memory

  bool add (int var) { return add (&var, 1); }              // Set a unit: true if var is positive or false if negative

  bool add (const int* in, unsigned int size) {             // Define a clause
    if (m_DB == 0 || in == 0 || size == 0) return false;    // Not allowed after clauses where deleted
    const int* clause = addClause (in, size, true);         // Add that clause to database
    if (size == 1 &&  m_false[ clause[0]]) return false;    // Check for conflicting unit
    if (size == 1 && !m_false[-clause[0]])                  // Check for a new unit
      assign (clause, true);                                // Directly assign new units (forced)
    return true; }

  template <typename Container>                             // Same as above, but a convenience function for STL containers
  bool add (const Container& v) {                           // A container has to have begin() and end()
    unsigned int size = 0;                                  // Such as std::vector, std::deque, std::set, std::list
    if (m_DB == 0) return false;                            // Not allowed after clauses where deleted
    typename Container::const_iterator i = v.begin();
    while (i != v.end() && *i != 0)                         // Plain copy to internal buffer, avoid zeros
      m_buffer[size++] = (int) *i++;
    return add (m_buffer, size); }                          // And call the other add() function

  //template <typename T>                                   // Uncomment if your compiler supports std::initializer_list
  //bool add (const std::initializer_list<T>& il) { return add(il); }

  bool solve (bool keepClauses = true) {                    // Determine satisfiability
    if (!m_DB) return m_model[0];                           // Already solved, return previous result
    int decision = m_head;                                  // Initialize the solver
    bool result = false;                                    // SAT if true, UNSAT if false
    while (true) {                                          // Main solve loop
      unsigned int old_nLemmas = m_nLemmas;                 // Store nLemmas to see whether propagate adds lemmas
      if (!propagate ()) break;                             // Propagation returns UNSAT for a root level conflict
      if (m_nLemmas > old_nLemmas) {                        // If the last decision caused a conflict
        decision = m_head;                                  // Reset the decision heuristic to head
        unsigned int threshold = (m_slow / 64) * 80;        // Restart threshold, same as 5/4 but more rounding
        if (m_fast > threshold) {                           // If fast average is substantially larger than slow average
          m_fast = threshold; restart ();                   // Restart and update the averages
          if (m_nLemmas > m_maxLemmas) reduceDB (); } }     // Reduce the DB when it contains too many lemmas

      while (m_false[+decision] || m_false[-decision])      // As long as the temporary decision is assigned
        decision = m_prev[decision];                        // Replace it with the next variable in the decision list

      if (decision == 0) { result = true; break; }          // If the end of the list is reached, then a solution is found
      decision = m_model[decision] ? +decision : -decision; // Otherwise, assign the decision variable based on the model
      m_false[-decision] = SAT;                             // Assign the decision literal to true (change to IMPLIED-1?)
      *(m_assigned++) = -decision;                          // And push it on the assigned stack
      decision = abs (decision); m_reason[decision] = 0; }  // Decisions have no reason clauses
      if (keepClauses) { restart (); reduceDB (0); }        // Remove all lemmas
      else             { delete[] m_DB; m_DB = 0; }         // Deallocate temporary memory
      m_model[0] = result; return result; }                 // And return result

  bool query (unsigned int var) const {                     // Return solution of a single variable
    return (int) var > m_nVars ? false : m_model[var]; }    // Return false for invalid variables
};
