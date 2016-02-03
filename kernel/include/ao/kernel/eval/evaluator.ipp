#include "ao/kernel/eval/evaluator.hpp"
#include "ao/kernel/eval/clause.hpp"
#include "ao/kernel/render/region.hpp"

#ifndef EVALUATOR_INCLUDE_IPP
#error "Cannot include .ipp file on its own"
#endif

////////////////////////////////////////////////////////////////////////////////
/*
 *  std::min and std::max misbehave when given Intervals, so we overload
 *  those functions with our own _min and _max (defined below for floats)
 */
inline float _min(const float& a, const float& b)
{
    return std::min(a, b);
}

inline float _max(const float& a, const float& b)
{
    return std::max(a, b);
}

inline float _abs(const float& a)
{
    return std::abs(a);
}

////////////////////////////////////////////////////////////////////////////////

#define EVAL_LOOP for (size_t i=0; i < count; ++i)

template <class T>
inline void evalClause(Opcode op, Result* a, Result* b, Result& result, size_t count)
{
    switch (op) {
        case OP_ADD:
            EVAL_LOOP
            result.set<T>(a->get<T>(i) +
                             b->get<T>(i), i);
            break;
        case OP_MUL:
            EVAL_LOOP
            result.set<T>(a->get<T>(i) *
                             b->get<T>(i), i);
            break;
        case OP_MIN:
            EVAL_LOOP
            result.set<T>(_min(a->get<T>(i),
                                  b->get<T>(i)), i);
            break;
        case OP_MAX:
            EVAL_LOOP
            result.set<T>(_max(a->get<T>(i),
                                  b->get<T>(i)), i);
            break;
        case OP_SUB:
            EVAL_LOOP
            result.set<T>(a->get<T>(i) -
                             b->get<T>(i), i);
            break;
        case OP_DIV:
            EVAL_LOOP
            result.set<T>(a->get<T>(i) /
                             b->get<T>(i), i);
            break;
        case OP_SQRT:
            EVAL_LOOP
            result.set<T>(sqrt(a->get<T>(i)), i);
            break;
        case OP_NEG:
            EVAL_LOOP
            result.set<T>(-a->get<T>(i), i);
            break;
        case OP_ABS:
            EVAL_LOOP
            result.set<T>(_abs(a->get<T>(i)), i);
            break;
        case OP_A:
            EVAL_LOOP
            result.set<T>(a->get<T>(i), i);
        case OP_B:
            EVAL_LOOP
            result.set<T>(b->get<T>(i), i);
        case INVALID:
        case OP_CONST:
        case OP_MUTABLE:
        case OP_X:
        case OP_Y:
        case OP_Z:
        case LAST_OP: assert(false);
    }
}
#undef EVAL_LOOP

#ifdef __AVX__
// Partial template specialization for SIMD evaluation
#define EVAL_LOOP for (size_t i=0; i <= (count - 1)/8; ++i)
template <>
inline void evalClause<__m256>(Opcode op, Result* __restrict a, Result* __restrict b, Result& result, size_t count)
{
    assert(count > 0);

    switch (op) {
        case OP_ADD:
            EVAL_LOOP
            result.set(_mm256_add_ps(a->get<__m256>(i),
                                        b->get<__m256>(i)), i);
            break;
        case OP_MUL:
            EVAL_LOOP
            result.set(_mm256_mul_ps(a->get<__m256>(i),
                                        b->get<__m256>(i)), i);
            break;
        case OP_MIN:
            EVAL_LOOP
            result.set(_mm256_min_ps(a->get<__m256>(i),
                                        b->get<__m256>(i)), i);
            break;
        case OP_MAX:
            EVAL_LOOP
            result.set(_mm256_max_ps(a->get<__m256>(i),
                                        b->get<__m256>(i)), i);
            break;
        case OP_SUB:
            EVAL_LOOP
            result.set(_mm256_sub_ps(a->get<__m256>(i),
                                        b->get<__m256>(i)), i);
            break;
        case OP_DIV:
            EVAL_LOOP
            result.set(_mm256_div_ps(a->get<__m256>(i),
                                        b->get<__m256>(i)), i);
            break;
        case OP_SQRT:
            EVAL_LOOP
            result.set(_mm256_sqrt_ps(a->get<__m256>(i)), i);
            break;
        case OP_NEG:
            EVAL_LOOP
            result.set(_mm256_mul_ps(a->get<__m256>(i),
                                        _mm256_set1_ps(-1)), i);
            break;
        case OP_ABS:
            EVAL_LOOP
            result.set(_mm256_andnot_ps(a->get<__m256>(i),
                                           _mm256_set1_ps(-0.0f)), i);
            break;
        case OP_A:
            EVAL_LOOP
            result.set(a->get<__m256>(i), i);
        case OP_B:
            EVAL_LOOP
            result.set(b->get<__m256>(i), i);
        case INVALID:
        case OP_CONST:
        case OP_MUTABLE:
        case OP_X:
        case OP_Y:
        case OP_Z:
        case LAST_OP: assert(false);
    }
}
#undef EVAL_LOOP
#endif

template <class T>
inline const T* Evaluator::evalCore(size_t count)
{
    for (const auto& row : rows)
    {
        for (size_t i=0; i < row.active; ++i)
        {
            Result* a = row[i]->a ? &row[i]->a->result : nullptr;
            Result* b = row[i]->b ? &row[i]->b->result : nullptr;
            evalClause<T>(row[i]->op, a, b, row[i]->result, count);
        }
    }
    return root->result.ptr<T>();
}

template <class T>
inline T Evaluator::eval(T x, T y, T z)
{
    setPoint(x, y, z, 0);
    return evalCore<T>(1)[0];
}

template <class T>
inline void Evaluator::setPoint(T x, T y, T z, size_t index)
{
    X->result.set<T>(x, index);
    Y->result.set<T>(y, index);
    Z->result.set<T>(z, index);
}

