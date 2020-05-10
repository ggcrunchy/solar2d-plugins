--- TODO
-- @classmod QueueRef

--
-- Permission is hereby granted, free of charge, to any person obtaining
-- a copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, sublicense, and/or sell copies of the Software, and to
-- permit persons to whom the Software is furnished to do so, subject to
-- the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
-- CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
-- TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--
-- [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
--

--- DOCME
-- @function QueueRef:Enqueue
-- @tparam Bytes data (as per Submit)
-- @ptable[opt] opts May include
-- * **align**: as per something, target blob
-- * **key**: expose this?
-- @treturn boolean OK?

--- DOCME
-- @function QueueRef:EnqueueBulk
-- @tparam {Bytes...} data (as per Submit)
-- @ptable[opt] opts May also include
-- * **count**: Number of elements to enqueue; by default, `#data`.
-- @treturn boolean OK?

--- DOCME
-- @function QueueRef:Size
-- @treturn uint approximate number of elements in queue (not exact since multiple threads might be hammering on it)

--- DOCME
-- @function QueueRef:TryDequeue
-- @ptable[opt] opts
-- * **key**: stuff
-- * **out**: If a **Blob**, used as output and return value (swapping per Submit)
-- @treturn ?|Blob|string|nil out

--- DOCME
-- @function QueueRef:TryDequeueBulk
-- @uint n
-- @ptable[opt] opts
-- * **key**: stuff
-- * **out**: If a table, used as output and return value, but corresponding **Blob** entries used as per TryDequeue
-- @treturn[1] uint N
-- @treturn[1] {Blob|string...}
-- @return[2] 0 for no result

--- DOCME (as per Enqueue but will fail if queue full)
-- @function QueueRef:TryEnqueue

--- DOCME (as per EnqueueBulk but will fail if queue full)
-- @function QueueRef:TryEnqueueBulk