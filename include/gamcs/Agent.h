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

/**
 * Format strings used for printing State and Action values regardless of platforms or INT_BITS
 */
#define ST_FMT IN_FMT		/**< usage: printf("%" ST_FMT "\n", state); */
#define ACT_FMT OUT_FMT		/**< usage: printf("%" ACT_FMT "\n", action); */

/**
 * The Intelligent Agent is a kind of TSGIOM which uses the Maximum Payoff Rule for constraining.
 * The Payoff is a real type value assigned to each state to indicate the like-dislike degree of that state by an agent.
 */
class Agent: public TSGIOM
{
	public:
		typedef GIOM::Input State; /**< for an agent we call an input a state */
		typedef GIOM::Output Action; /**< for an agent we call an output an action*/
		typedef GIOM::Output EnvAction; /**< the environment action */

		enum Mode
		{
			ONLINE = 0, /**< in this mode, an agent will learn as it acts, and uses what it has learned to guide its action, this is the default mode */
			EXPLORE /**< in this mode, an agent will learn as it acts, but not use what it has learned, instead it will act randomly */
		};

		Agent(int id = 0, float discount_rate = 0.9, float threshold = 0.01);
		virtual ~Agent();

		void setMode(Mode mode);
		void update(float original_payoff);

		static const State INVALID_STATE; /**< the invalid state indicator */
		static const Action INVALID_ACTION; /**< the invalid action indicator */
		static const float INVALID_PAYOFF; /**< the invalid payoff indicator */

	protected:
		int id; /**< the id of an agent */
		float discount_rate; /**< the discount rate [0,1) used to calculate state payoff */
		float threshold; /**< the threshold used to update payoff */
		Mode learning_mode; /**< the learning mode, ONLINE by default */

		OSpace constrain(State state, OSpace &avaliable_actions) const;

		/** These two functions are implementation dependent, declared as pure virtual functions */
		virtual OSpace maxPayoffRule(State state,
				OSpace &available_actions) const = 0; /**< implementation of maximum payoff rule */
		virtual void updateMemory(float original_payoff) = 0; /**<  update states in memory given the original payoff of current state */
};

/*
 * To support dumping and loading of agent memory between different platforms, the arrangement strategy must be set as the same.
 * */
#pragma pack(push) // save arrangement value
#pragma pack(2)	// set arrangement value to 2

/** Action information header */
struct Action_Info_Header
{
		Agent::Action act; /**< the action value */
		uint32_t eat_num; /**< the number of environment actions which have been observed under this action */
};

/** Environment Action information */
struct EnvAction_Info
{
		Agent::EnvAction eat; /**< the environment action value */
		uint32_t count; /**< the experiencing counts of this environment action */
		Agent::State nst; /**< the following state value of this environment action */
};

/** State information header
 *
 * The structure of a state information is like this:
 *
 * ------------------------------------------------------------------------------------------------------------------------------------
 * |				   |					|				 |				  |		|					 |				  |		|	  |
 * | State_Info_Header | Action_Info_Header | EnvAction_Info | EnvAction_Info | ... | Action_Info_Header | EnvAction_Info | ... | ... |
 * |				   |					|				 |				  |		|					 |				  |		|	  |
 * ------------------------------------------------------------------------------------------------------------------------------------
 * */
struct State_Info_Header
{
		Agent::State st; /**< the state value */
		float original_payoff; /**< the original payoff, NOTE: float is assumed to be 4 bits for all platforms */
		float payoff; /**< the calculated payoff */
		uint32_t count; /**< counts of traveling through this state */
		uint32_t act_num; /**< number of actions which have been performed under this state */
		uint16_t size; /**< size of the header (in Byte) */
};

/** Memory information */
struct Memory_Info
{
		float discount_rate; /**< the discount rate */
		float threshold; /**< the threshold */
		uint32_t state_num; /**< total number of states in memory */
		uint32_t lk_num; /**< total number of links between states in memory */
		Agent::State last_st; /**< the last experienced state when dumping memory */
		Agent::Action last_act; /**< the last performed action when dumping memory */
};

#pragma pack(pop)	// pop saved default value

}    // namespace gamcs
#endif // AGENT_H
