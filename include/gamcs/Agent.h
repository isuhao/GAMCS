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

#ifndef AGENT_H
#define AGENT_H
#include "gamcs/TSGIOM.h"
#include "gamcs/debug.h"

namespace gamcs
{

/*
 * Format strings used for printing State and Action values regardless of platforms or INT_BITS
 */
/**< usage: \code{.cpp} printf("%" ST_FMT "\n", state); \endcode */
#define ST_FMT IN_FMT
/**< usage: \code{.cpp} printf("%" ACT_FMT "\n", action); \endcode */
#define ACT_FMT OUT_FMT

/**
 * @brief The Intelligent Agent is a kind of TSGIOM which uses the Maximum Payoff Rule for constraining.
 *
 * The Payoff is a real type value assigned to each state to indicate the like-dislike degree of that state by an agent.
 */
class Agent: public TSGIOM
{
	public:
		typedef GIOM::Input State; /**< for an agent we call an input a state <br> Print: @code printf("%" ST_FMT "\n", state); @endcode  */
		typedef GIOM::Output Action; /**< for an agent we call an output an action <br> Print: @code printf("%" ACT_FMT "\n", action); @endcode */
		typedef GIOM::Output EnvAction; /**< the environment action <br> Print: @code printf("%" ACT_FMT "\n", eact); @endcode */

		/**
		 * Learning mode of the agent.
		 */
		enum Mode
		{
			ONLINE = 0, /**< in this mode, an agent will learn as it acts, and uses what it has learned to guide its action, this is the default mode */
			EXPLORE, /**< in this mode, an agent will learn as it acts, but not use what it has learned, instead it will act randomly */
			TEACH
		};

		Agent(int id = 0, float discount_rate = 0.9, float accuracy = 0.01);
		virtual ~Agent();

		void setMode(Mode mode);
		Mode getMode();
		void update(float original_payoff);

		static const State INVALID_STATE; /**< the invalid state indicator */
		static const Action INVALID_ACTION; /**< the invalid action indicator */
		static const float INVALID_PAYOFF; /**< the invalid payoff indicator */

	protected:
		int id; /**< the id of an agent */
		float discount_rate; /**< the discount rate [0,1) used to calculate state payoff */
		float accuracy; /**< the accuracy of payoff, ranging [0, +inf) */
		Mode learning_mode; /**< the learning mode, ONLINE by default */

		OSpace constrain(State state, OSpace &avaliable_actions) const;

		/* These two functions are implementation dependent, declared as pure virtual functions */
		/**
		 * @brief The Maximum Payoff Rule (MPR).
		 *
		 * @param [in] state the state which is concerned
		 * @param [in] available_actions the action space of the state
		 * @return the actions generated by MPR
		 */
		virtual OSpace maxPayoffRule(State state,
				OSpace &available_actions) const = 0;

		/**
		 * @brief Update states in memory given the original payoff of current state.
		 *
		 * @param [in] original_payoff original payoff of current state
		 */
		virtual void updateMemory(float original_payoff) = 0;
};

/*
 * To support dumping and loading of agent memory between different platforms, the arrangement strategy must be set as the same.
 * */
#pragma pack(push) // save arrangement value
#pragma pack(2)	// set arrangement value to 2

/**
 * @brief Action information header
 */
struct Action_Info_Header
{
		Agent::Action act; /**< the action value */
		uint32_t eat_num; /**< the number of environment actions which have been observed under this action */
};

/**
 * @brief Environment Action information
 */
struct EnvAction_Info
{
		Agent::EnvAction eat; /**< the environment action value */
		uint32_t count; /**< the experiencing counts of this environment action */
		Agent::State nst; /**< the following state value of this environment action */
};

/* ------------------------------------------------------------------------------------------------------------------------------------
 * |				   |					|				 |				  |		|					 |				  |		|	  |
 * | State_Info_Header | Action_Info_Header | EnvAction_Info | EnvAction_Info | ... | Action_Info_Header | EnvAction_Info | ... | ... |
 * |				   |					|				 |				  |		|					 |				  |		|	  |
 * ------------------------------------------------------------------------------------------------------------------------------------
 */
/**
 * @brief State information header
 *
 * The structure of a state information is like this:
 * @dot
 * digraph stif {
 *     node [ fontsize = "16" shape = "rectangle"];
 *     edge [];
 *
 *     "pstif" [label = "State_Info_Header *"];
 *
 *     "stif" [label = "<st> State_Info_Header|<act> Action_Info_Header|<eat> EnvAction_Info|EnvAction_Info|...|Action_Info_Header|EnvAction_Info|...|Action_Info_Header|...|..." shape = "record"];
 *
 *     "st" [label = "{<hd> st|original payoff|payoff|count|act_num|size}" shape = "record"];
 *     "act" [label = "{<hd> act|eat_num}" shape = "record"];
 *     "eat" [label = "{<hd> eat|count|nst}" shape = "record"];
 *
 *     "pstif" -> "stif":st;
 *     "stif":st -> "st":hd;
 *     "stif":act -> "act":hd;
 *     "stif":eat -> "eat":hd;
 * }
 * @enddot
 */
struct State_Info_Header
{
		Agent::State st; /**< the state value */
		float original_payoff; /**< the original payoff, NOTE: float is assumed to be 4 bits for all platforms */
		float payoff; /**< the calculated payoff */
		uint32_t count; /**< counts of traveling through this state */
		uint32_t act_num; /**< number of actions which have been performed under this state */
		uint16_t size; /**< size of the header (in Byte) */
};

/**
 * @brief Memory information
 */
struct Memory_Info
{
		float discount_rate; /**< the discount rate */
		float accuracy; /**< the accuracy */
		uint32_t state_num; /**< total number of states in memory */
		uint32_t lk_num; /**< total number of links between states in memory */
		Agent::State last_st; /**< the last experienced state when dumping memory */
		Agent::Action last_act; /**< the last performed action when dumping memory */
};

#pragma pack(pop)	// pop saved default value

}    // namespace gamcs
#endif // AGENT_H
