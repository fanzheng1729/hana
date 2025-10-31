#ifndef GOM_H_INCLUDED
#define GOM_H_INCLUDED

#include <algorithm>// for std::copy and std::fill
#include <cstddef>  // for std::size_t and NULL
#include <cstring>  // for std::memcmp
#include <iostream>
#include <utility>
#include <vector>

// M * N Gomoko, with K stones in a row as a win.
template<std::size_t M, std::size_t N, std::size_t K>
struct Gom
{
    enum Player { US = 1, THEM = -1, NONE = 0 };
    // The board
    int board[M][N];
    // Move = (row, column)
    typedef std::pair<std::size_t, std::size_t> Move;
    // Vector of moves
    typedef std::vector<Move> Moves;
    // Constructor
    Gom(int const p[] = NULL)
    {
        if (p)  std::copy(p, p + M * N, board[0]);
        else    std::fill(board[0], board[0] + M * N, NONE);
    }
    // Return if the board is full.
    bool full() const { return std::find(board[0], board[M], 0) == board[M]; }
    friend std::ostream & operator<<(std::ostream & out, Gom const& gom)
    {
        for (std::size_t i = 0; i < M; ++i)
        {
            for (std::size_t j = 0; j < N; ++j)
                out << "X.O"[gom.board[i][j] + 1];
            out << std::endl;
        }
        return out;
    }
    // Return the player with a run-of-K from begin to end with step apart.
    static int run(const int * begin, const int * end, std::size_t step)
    {
        std::size_t count = 0;
        int who = NONE;

        for ( ; begin < end && count < K; begin += step)
            *begin == NONE ? count = 0 : // Reset
                count == 0 ? (who = *begin, ++count) : // Start
                    *begin == who ? ++count : // Continue
                        count = 0; // Reset

        return count == K ? who : NONE;
    }
    // Return the player with a run-of-K.
    static int run(const int * begin, const int * end, std::size_t step,
                   long beginstep, long endstep, std::size_t count)
    {
        for ( ; count > 0; --count)
        {
            int const who = run(begin, end, step);
            if (who) return who;

            begin += beginstep;
            end += endstep;
        }

        return NONE;
    }
    int winner() const
    {
        int who = NONE;
        // Check rows.
        who = run(&board[0][0], &board[1][0], 1, N, N, M);
        if (who) return who;
        // Check columns.
        who = run(&board[0][0], &board[M][0], N, 1, 1, N);
        if (who) return who;
        // No winner.
        return 0;
    }
    // Return all the legal moves.
    Moves moves(bool = true) const
    {
        Moves result;
        for (std::size_t i = 0; i < M; ++i)
            for (std::size_t j = 0; j < N; ++j)
                if (board[i][j] == 0)
                    result.push_back(Move(i, j));
        return result;
    }
    bool legal(Move move, bool) const
    {
        return move.first < M && move.second < N && board[move.first][move.second] == 0;
    }
    Gom play(Move move, bool ourturn) const
    {
        Gom result(*this);
        result.board[move.first][move.second] = ourturn ? US : THEM;
        return result;
    }
};
template<std::size_t M, std::size_t N, std::size_t K>
inline bool operator==(Gom<M,N,K> const & x, Gom<M,N,K> const & y)
    { return std::memcmp(x.board[0], y.board[0], sizeof(Gom<M,N,K>::board)) == 0; }
template<std::size_t M, std::size_t N, std::size_t K>
inline bool operator!=(Gom<M,N,K> const & x, Gom<M,N,K> const & y)
    { return !(x == y); }

#endif // GOM_H_INCLUDED
