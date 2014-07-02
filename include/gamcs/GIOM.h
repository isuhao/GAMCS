// -----------------------------------------------------------------------------
//
// GAMCS -- Generalized Agent Model and Computer Simulation
//
// Copyright (C) 2013-2014, Andy Huang  <andyspider@126.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// -----------------------------------------------------------------------------
//
// Created on: Oct 19, 2013
//
// -----------------------------------------------------------------------------

#ifndef GIOM_H
#define GIOM_H
#include <stddef.h>     // NULL
#define __STDC_LIMIT_MACROS     // UINT64_MAX
#include <cstdint>
#define __STDC_FORMAT_MACROS    // PRIu64

#if defined( __GNUC__) || defined(__linux)
#include <inttypes.h>
#endif
#include "gamcs/config.h"
#include "gamcs/debug.h"
#include "gamcs/platforms.h"

namespace std
{
class random_device;
}

namespace gamcs
{

/**
 * Define signed and unsigned integer types according to INT_BITS.
 */
#if INT_BITS == 8
typedef uint8_t gamcs_uint;
typedef int8_t gamcs_int;
#define GAMCS_UINT_FMT UINT8_FMT
#define GAMCS_INT_FMT INT8_FMT
#define GAMCS_UINT_MAX UINT8_MAX
#define GAMCS_INT_MAX INT8_MAX

#elif INT_BITS == 16
typedef uint16_t gamcs_uint;
typedef int16_t gamcs_int;
#define GAMCS_UINT_FMT UINT16_FMT
#define GAMCS_INT_FMT INT16_FMT
#define GAMCS_UINT_MAX UINT16_MAX
#define GAMCS_INT_MAX INT16_MAX

#elif INT_BITS == 32
typedef uint32_t gamcs_uint;
typedef int32_t gamcs_int;
#define GAMCS_UINT_FMT UINT32_FMT
#define GAMCS_INT_FMT INT32_FMT
#define GAMCS_UINT_MAX UINT32_MAX
#define GAMCS_INT_MAX INT32_MAX

#else       /* 64bit by default */
typedef uint64_t gamcs_uint;
typedef int64_t gamcs_int;
#define GAMCS_UINT_FMT UINT64_FMT
#define GAMCS_INT_FMT INT64_FMT
#define GAMCS_UINT_MAX UINT64_MAX
#define GAMCS_INT_MAX INT64_MAX

#endif

/*
 * Format strings used for printing Input and Output values regardless of platforms or INT_BITS
 */
/**< usage: @code{.cpp} printf("%" IN_FMT "\n", input); @endcode */
#define IN_FMT GAMCS_INT_FMT
/**< usage: @code{.cpp} printf("%" OUT_FMT "\n", output); @endcode */
#define OUT_FMT GAMCS_INT_FMT

class OSpace;

/**
 * @brief Generalized Input Output Model
 */
class GIOM
{
	public:
		typedef gamcs_int Input; /**< Input type (signed integer) <br> Print: @code printf("%" IN_FMT "\n", input); @endcode */
		typedef gamcs_int Output; /**< Output type (signed integer) <br> Print:  @code printf("%" OUT_FMT "\n", output); @endcode */

		GIOM();
		virtual ~GIOM();

		Output process(Input input, OSpace &available_outputs);
		float singleOutputEntropy(Input input, OSpace &available_outputs) const;
		virtual void update();

		static const Input INVALID_INPUT = GAMCS_INT_MAX; /**< the maximum value is used to indicate an invalid input */
		static const Output INVALID_OUTPUT = GAMCS_INT_MAX; /**< the maximum value is used to indicate an invalid output */

	protected:
		virtual OSpace constrain(Input input, OSpace &available_outputs) const;
		Input cur_in; /**< current input */
		Output cur_out; /**< current output corresponding to cur_in */
		unsigned long process_count; /**< processing counts */

	private:
		std::random_device *rand_device; /**< random-generating engine */
		unsigned long max_rand_value; /**< maximum random value possibly generated by rand_device */
		gamcs_uint randomGenerator(gamcs_uint size) const;
};

/**
 * @brief Fragment is used to store a single output or a range of outputs.
 *
 * For example: output space {1, 3, 5, 7 ,9} can be represented as [1, 9, 2] by an OFragment.
 */
struct OFragment
{
		GIOM::Output start; /**< the starting output */
		GIOM::Output end; /**< the ending output, for a single output, the ending is equal to the starting. */
		GIOM::Output step; /**< the increasing or decreasing step */
};

/**
 * @brief Output space which contains a series of outputs.
 *
 *  The space is different from a set in which outputs can be repeated.
 *  Repeated outputs will be counted more than once, this can be used
 *  to mimic the weight of an output.
 *  The more times it appears, the bigger weight it has.
 */
class OSpace
{
	public:
		/**
		 * The spare capacity.
		 */
		enum
		{
			SPARE_CAPACITY = 5 /**< SPARE_CAPACITY */
		};

		typedef gamcs_uint ossize_t; /**< output space size type */

		/**
		 * @brief The default constructor.
		 *
		 * @param [in] initfn the initial number of fragments
		 */
		explicit OSpace(ossize_t initfn = 0) :
				frag_num(initfn), the_capacity(initfn + SPARE_CAPACITY), output_num(
						0), current_index(0), outputs(NULL)
		{
			outputs = new OFragment[the_capacity];
		}

		/**
		 * @brief The default copy constructor.
		 *
		 * @param [in] other another OSpace object
		 */
		OSpace(const OSpace &other) :
				frag_num(0), the_capacity(SPARE_CAPACITY), output_num(0), current_index(
						0), outputs(NULL)
		{
			operator=(other);
		}

		/**
		 * @brief The default destructor.
		 */
		~OSpace()
		{
			delete[] outputs;
		}

		/**
		 * @brief Check if the space is empty.
		 *
		 * @return true or false
		 */
		bool empty() const
		{
			return (size() == 0);
		}

		/**
		 * @brief Get the total number of outputs in the space.
		 *
		 * @return  the number
		 */
		ossize_t size() const
		{
			return output_num;
		}

		/**
		 * @brief Get the current capacity of the space.
		 *
		 * @return the capacity
		 */
		ossize_t capacity() const
		{
			return the_capacity;
		}

		/**
		 * @brief Override operator [].
		 *
		 * For index that is out of bound, INVALID_OUTPUT is returned.
		 * @param [in] index the output index (starting from 0)
		 * @return the output value at that index
		 */
		GIOM::Output operator[](ossize_t index) const
		{
			ossize_t total_num = 0;
			ossize_t out_wanted;

			if (index < total_num)    // subscript out of bound
			{
				return GIOM::INVALID_OUTPUT;
			}

			ossize_t i;
			// traverse each fragment to find which fragment the index locates in
			for (i = 0; i < frag_num; i++)
			{
				OFragment *ptr = outputs + i;
				ossize_t out_num_in_this_frag = (ptr->end - ptr->start)
						/ ptr->step + 1;
				if (total_num + out_num_in_this_frag > index)    // index is in this fragment
				{
					ossize_t index_in_frag = index - total_num;
					out_wanted = ptr->start + ptr->step * index_in_frag;
					break;
				}

				total_num += out_num_in_this_frag;
			}

			if (i >= frag_num)    // superscript out of bound
			{
				return GIOM::INVALID_OUTPUT;
			}

			return out_wanted;
		}

		/**
		 * @brief Override operator=.
		 *
		 * @param [in] other another OSpace object
		 * @return the reassigned object
		 */
		const OSpace &operator=(const OSpace &other)
		{
			if (this != &other)
			{
				delete[] outputs;
				// copy data
				frag_num = other.frag_num;
				the_capacity = other.the_capacity;
				output_num = other.output_num;

				outputs = new OFragment[the_capacity];
				// copy each fragment
				for (ossize_t i = 0; i < frag_num; i++)
				{
					outputs[i] = other.outputs[i];
				}
			}

			return *this;
		}

		/**
		 * @brief Add a single output to the space.
		 *
		 * @param output the output
		 */
		void add(GIOM::Output output)
		{
			// check if exceeds the capacity
			if (frag_num == the_capacity)
				expand(2 * the_capacity + 1);

			OFragment new_frag;
			new_frag.start = output;
			new_frag.end = output;
			new_frag.step = 1;
			outputs[frag_num++] = new_frag;    // copy fragment and increase num
			++output_num;
		}

		/**
		 * @brief Add an output range to the space.
		 *
		 * @param [in] start the starting output
		 * @param [in] end the ending output
		 * @param step the increasing or decreasing step
		 */
		void add(GIOM::Output start, GIOM::Output end, GIOM::Output step)
		{
			// check range
			if ((end - start) / step < 0)
				ERROR(
						"Invalid range! %" OUT_FMT " --> %" OUT_FMT " (step: %" OUT_FMT ") \n",
						start, end, step);

			if (frag_num == the_capacity)
				expand(2 * the_capacity);

			OFragment new_frag;
			new_frag.start = start;
			new_frag.end = end;
			new_frag.step = step;
			outputs[frag_num++] = new_frag;
			output_num += (end - start) / step + 1;
		}

		/**
		 * @brief Expand the space capacity.
		 *
		 * @param ncap the requested new capacity
		 */
		void expand(ossize_t ncap)
		{
			if (ncap < frag_num)
				return;    // the new capacity should at least include all current fragments

			OFragment *old_list = outputs;
			// add up the spare
			ncap += SPARE_CAPACITY;
			outputs = new OFragment[ncap];
			// copy all fragments to new place
			for (ossize_t i = 0; i < frag_num; i++)
			{
				outputs[i] = old_list[i];
			}

			the_capacity = ncap;
			delete[] old_list;
		}

		/**
		 * @brief Clear the space.
		 */
		void clear()
		{
			frag_num = 0;
			output_num = 0;
			current_index = 0;
		}

		/**
		 * @brief Get the first output in space.
		 *
		 * @return the first output
		 */
		GIOM::Output first() const
		{
			current_index = 0;
			return operator[](current_index);
		}

		/**
		 * @brief Get the last output in space.
		 *
		 * @return the last output
		 */
		GIOM::Output last() const
		{
			return operator[](size() - 1);
		}

		/**
		 * @brief Iterate the space and get the next output.
		 *
		 * @return the next output
		 */
		GIOM::Output next() const
		{
			current_index++;
			return operator[](current_index);
		}

	private:
		ossize_t frag_num; /**< the number of fragments */
		ossize_t the_capacity; /**< the space capacity */
		ossize_t output_num; /**< the number of outputs */
		mutable ossize_t current_index; /**< the index used by iterator */
		OFragment *outputs; /**< OFragments used to store outputs*/
};

}    // namespace gamcs
#endif // GIOM_H
