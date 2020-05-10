--- Wrapper over Eigen's matrices.
--
-- In addition to those listed below, matrices have those methods listed in [CommonWriteOps](./commonwriteops.html),
-- [NumericalWriteOps](./numericalwriteops.html), and [XprOps](./xprops.html).

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

--- DOCMEMORE
-- @function Matrix:acos
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:add
-- @tparam Matrix B
-- @string? vectorwise
-- @treturn Matrix A

--- Metamethod
-- May be matrix / scalar combo
-- @function Matrix:__add
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:adjoint
-- @treturn Matrix A

--- DOCMEMORE
-- @function Matrix:allFinite
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:arg
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:asDiagonal
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:asMatrix
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:asPermutation
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:asin
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:atan
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:bdcSvd
-- @ptable[opt] opts ("FullU", "ThinU", "FullV", "ThinV")
-- @treturn BDCSVD B

--- DOCMEMORE
-- @function Matrix:binaryExpr
-- @callable func
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:blueNorm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:__call
-- @uint row
-- @uint? col
-- @treturn Scalar x

--- DOCMEMORE
-- @function Matrix:cast
-- @string type
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:ceil
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:colPivHouseholderQr
-- @treturn ColPivHouseholderQR C

--- DOCMEMORE
-- @function Matrix:cols
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:colStride
-- @treturn uint C

--- DOCEMORE
-- @function Matrix:completeOrthogonalDecomposition
-- @treturn CompleteOrthogonalDecomposition C

--- DOCMEMORE
-- @function Matrix:conjugate
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:conservativeResize
-- @uint nrows
-- @uint? ncols

--- DOCMEMORE
-- @function Matrix:conservativeResizeLike
-- @tparam Matrix other

--- DOCMEMORE
-- @function Matrix:cos
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cosh
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cube
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseAbs
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseAbs2
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseEqual
-- @tparam ?|Matrix|Scalar m
-- @treturn BoolMatrix B

--- DOCMEMORE
-- @function Matrix:cwiseGreaterThan
-- @tparam ?|Matrix|Scalar m
-- @treturn BoolMatrix B

--- DOCMEMORE
-- @function Matrix:cwiseGreaterThanOrEqual
-- @tparam ?|Matrix|Scalar m
-- @treturn BoolMatrix B

--- DOCMEMORE
-- @function Matrix:cwiseInverse
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseLessThan
-- @tparam ?|Matrix|Scalar m
-- @treturn BoolMatrix B

--- DOCMEMORE
-- @function Matrix:cwiseLessThanOrEqual
-- @tparam ?|Matrix|Scalar m
-- @treturn BoolMatrix B

--- DOCMEMORE
-- @function Matrix:cwiseMax
-- @tparam ?|Matrix|Scalar m
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseMin
-- @tparam ?|Matrix|Scalar m
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseNotEqual
-- @tparam ?|Matrix|Scalar m
-- @treturn BoolMatrix B

--- DOCMEMORE
-- @function Matrix:cwiseProduct
-- @tparam Matrix M
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseQuotient
-- @tparam Matrix M
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseSign
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:cwiseSqrt
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:data
-- @treturn userdata D

--- DOCMEMORE
-- @function Matrix:determinant
-- @treturn number x

--- DOCMEMORE
-- @function Matrix:diagonalSize
-- @treturn uint C

--- Metamethod
-- RHS may be matrix / scalar combo
-- @function Matrix:__div
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:dot
-- @tparam Matrix other
-- @treturn Scalar x

--- DOCMEMORE
-- @function Matrix:eigenSolver
-- @string? N ("NoEigenvalues")
-- @treturn ?|EigenSolver|ComplexEigenSolver ES

--- DOCMEMORE
-- @function Matrix:exp
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:floor
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:fullPivHouseholderQr
-- @treturn FullPivHouseholderQR F

--- DOCMEMORE
-- @function Matrix:fullPivLu
-- @treturn FullPivLU F

--- DOCMEMORE
-- @function Matrix:generalizedEigenSolver
-- @tparam Matrix R
-- @string? N ("NoEigenvectors")
-- @treturn GeneralizedEigenSolver GES

--- DOCMEMORE
-- @function Matrix:generalizedSelfAdjointEigenSolver
-- @tparam Matrix R
-- @ptable[opt] opts ( no_eigenvectors = bool, method = "ABx_lx", "Ax_lBx", "BAx_lx" )
-- @treturn GeneralizedSelfAdjointEigenSolver G

--- DOCMEMORE
-- @function Matrix:hasNaN
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:hessenbergDecomposition
-- @treturn HessenbergDecomposition H

--- DOCMEMORE
-- @function Matrix:householderQr
-- @treturn HouseholderQR H

--- DOCMEMORE
-- @function Matrix:hypotNorm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:imag
-- @treturn Matrix M

--- DOCMEMORE
-- @function Matrix:imagAssign
-- @tparam Matrix M

--- DOCMEMORE
-- @function Matrix:innerSize
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:innerStride
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:inverse
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:isApprox
-- @tparam Matrix A
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isConstant
-- @tparam Scalar S
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isDiagonal
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isFinite
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isIdentity
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isInf
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isLowerTriangular
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isMuchSmallerThan
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isNaN
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isOnes
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isOrthogonal
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isUnitary
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isUpperTriangular
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:isZero
-- @number? prec
-- @treturn boolean B

--- DOCMEMORE
-- @function Matrix:jacobiSvd
-- @ptable[opt] opts (Follows bdcSvd, also has preconditioner = "fullPiv", "householder", "none")
-- @treturn JacobiSVD J

--- DOCMEMORE
-- @function Matrix:ldlt
-- @string? UL ("upper")
-- @treturn LDLT L

--- Metamethod.
--
-- Synonym for @{Matrix:size}.
-- @function Matrix:__len
-- @treturn uint Number of elements.

--- DOCMEMORE
-- @function Matrix:llt
-- @string? UL ("upper")
-- @treturn LLT L

--- DOCMEMORE
-- @function Matrix:log
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:log10
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:lp1Norm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:lpInfNorm
-- @treturn number n

--- Synonym for @{Matrix:partialPivLu}.
-- @function Matrix:lu

--- DOCMEMORE
-- @function Matrix:maxCoeff
-- @string? choice (colwise, rowwise)
-- @treturn[1] Scalar S
-- @treturn[2] Matrix M (col, row)

--- DOCMEMORE
-- @function Matrix:mean
-- @string? choice (colwise, rowwise)
-- @treturn[1] Scalar S
-- @treturn[2] Matrix M (col, row)

--- DOCMEMORE
-- @function Matrix:minCoeff
-- @string? choice (colwise, rowwise)
-- @treturn[1] Scalar S
-- @treturn[2] Matrix M (col, row)

--- Metamethod
-- May be matrix / scalar combo
-- @function Matrix:__mul
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:norm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:normalized
-- @string? vectorwise
-- @treturn Matrix A

--- DOCMEMORE
-- @function Matrix:operatorNorm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:outerSize
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:outerStride
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:partialPivLu
-- @treturn PartialPivLU P

--- Metamethod
-- May be matrix / scalar combo
-- @function Matrix:__pow
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:prod
-- @string? choice (colwise, rowwise)
-- @treturn[1] Scalar S
-- @treturn[2] Matrix M (col, row)

--- DOCMEMORE
-- @function Matrix:real
-- @treturn Matrix M

--- DOCMEMORE
-- @function Matrix:realAssign
-- @tparam Matrix M

--- DOCMEMORE
-- @function Matrix:realQz
-- @tparam Matrix R
-- @string? N ("NoQZ")
-- @treturn RealQZ RQZ

--- DOCMEMORE
-- @function Matrix:redux
-- @callable func
-- @param how
-- @treturn[1] Matrix x
-- @return[2] Scalar y

--- DOCMEMORE
-- @function Matrix:replicate
-- @uint vcount
-- @?|uint|string hcount / choice (colwise, rowwise)
-- @treturn Matrix R

--- DOCMEMORE
-- @function Matrix:reshape
-- @uint nrows
-- @uint ncols
-- @treturn Map

--- DOCMEMORE
-- @function Matrix:reshapeWithInnerStride
-- @uint nrows
-- @uint ncols
-- @uint stride
-- @treturn Map

--- DOCMEMORE
-- @function Matrix:reshapeWithOuterStride
-- @uint nrows
-- @uint ncols
-- @uint stride
-- @treturn Map

--- DOCMEMORE
-- @function Matrix:resize
-- @uint nrows
-- @uint? ncols

--- DOCMEMORE
-- @function Matrix:resizeLike
-- @tparam Matrix other

--- DOCMEMORE
-- @function Matrix:reverse
-- @string? vectorwise
-- @treturn Matrix A

--- DOCMEMORE
-- @function Matrix:round
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:rows
-- @treturn uint R

--- DOCMEMORE
-- @function Matrix:rowStride
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:schur
-- @string? N ("NoU")
-- @treturn ?|RealSchur|ComplexSchur S

--- DOCMEMORE
-- @function Matrix:selfAdjointEigenSolver
-- @string? N ("NoEigenvectors")
-- @treturn SelfAdjointEigenSolver S

--- DOCMEMORE
-- @function Matrix:selfAdjointView
-- @string N ("Lower", "Upper")
-- @treturn SelfAdjointView S

--- DOCMEMORE
-- @function Matrix:sin
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:sinh
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:size
-- @treturn uint C

--- DOCMEMORE
-- @function Matrix:square
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:squaredNorm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:stableNorm
-- @treturn number n

--- DOCMEMORE
-- @function Matrix:stableNormalized
-- @treturn Matrix A

--- DOCMEMORE
-- @function Matrix:sub
-- @tparam Matrix B
-- @string? vectorwise
-- @treturn Matrix A

--- Metamethod
-- May be matrix / scalar combo
-- @function Matrix:__sub
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:sum
-- @string? choice (colwise, rowwise)
-- @treturn[1] Scalar S
-- @treturn[2] Matrix M (col, row)

--- DOCMEMORE
-- @function Matrix:tan
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:tanh
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:__tostring

--- DOCMEMORE
-- @function Matrix:trace
-- @treturn Scalar x

--- DOCMEMORE
-- @function Matrix:transpose
-- @treturn Transpose A

--- DOCMEMORE
-- @function Matrix:triangularView
-- @string ("Lower", "Upper", "StrictlyLower", "StrictlyUpper", "UnitLower", "UnitUpper")
-- @treturn TriangularView T

--- DOCMEMORE
-- @function Matrix:tridiagonalization
-- @treturn Tridiagonalization T

--- DOCMEMORE
-- @function Matrix:unaryExpr
-- @callable func
-- @treturn Matrix x

--- DOCMEMORE
-- @function Matrix:unitOrthogonal
-- @return Matrix O

--- Metamethod
-- @function Matrix:__unm
-- @treturn Matrix S

--- DOCMEMORE
-- @function Matrix:value
-- @treturn Scalar x

--- DOCMEMORE
-- @function Matrix:visit
-- @callable init
-- @callable rest