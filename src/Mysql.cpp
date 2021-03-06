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

#if defined( _WIN32)
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "gamcs/Mysql.h"
#include "gamcs/debug.h"

namespace gamcs
{

/**
 * @brief The default constructor.
 *
 * @param [in] server the mysql server address
 * @param [in] user the mysql user name
 * @param [in] password the mysql password
 * @param [in] dbname the database name
 */
Mysql::Mysql(std::string server, std::string user, std::string password,
		std::string dbname) :
		db_con(NULL), db_server(server), db_user(user), db_password(password), db_name(
				dbname), db_t_stateinfo("StateInfo"), db_t_meminfo(
				"MemoryInfo"), current_index(0)
{
}

/**
 * @brief The default destructor.
 */
Mysql::~Mysql()
{
}

/**
 * @brief Set mysql arguments.
 *
 * @param [in] srv the mysql server address
 * @param [in] usr the mysql user name
 * @param [in] passwd the mysql password
 * @param [in] db the database name
 */
void Mysql::setDBArgs(std::string srv, std::string usr, std::string passwd,
		std::string db)
{
	db_server = srv;
	db_user = usr;
	db_password = passwd;
	db_name = db;
	return;
}

/**
 * @brief Open the storage for read or write.
 *
 * @param [in] flag the open flag
 * @return 0 on successfully opened, or -1 if error occurs
 */
int Mysql::open(Flag flag)
{
	if (mysql_library_init(0, NULL, NULL))
	{
		fprintf(stderr, "could not initialize MySQL library!\n");
		return -1;
	}

	db_con = mysql_init(NULL);

	if (db_con == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return -1;
	}

	if (mysql_real_connect(db_con, db_server.c_str(), db_user.c_str(),
			db_password.c_str(), NULL, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return -1;
	}

	// open database
	int ret = 0;
	if (flag == O_READ)    // open for reading
	{
		ret = mysql_select_db(db_con, db_name.c_str());    // use database
		if (ret)	// select error
		{
			fprintf(stderr, "Can't select mysql database for reading, %s\n",
					mysql_error(db_con));
			return ret;
		}
	}
	else if (flag == O_WRITE)    // open for writing
	{

		/* create database if not exists */
		char db_string[128];
		sprintf(db_string, "CREATE DATABASE IF NOT EXISTS %s", db_name.c_str());
		if (mysql_query(db_con, db_string))
		{
			fprintf(stderr, "Can't create database, %s\n", mysql_error(db_con));
			return -1;
		}

		ret = mysql_select_db(db_con, db_name.c_str());    // use database
		if (ret)	// select error
		{
			fprintf(stderr, "Can't select mysql database for writing, %s\n",
					mysql_error(db_con));
			return ret;
		}

		/* create table if not exists */
		char tb_string[256];
		sprintf(tb_string,
				"CREATE TABLE IF NOT EXISTS %s.%s(State BIGINT PRIMARY KEY, OriPayoff FLOAT, Payoff FLOAT, Count BIGINT, ActNum BIGINT, Size INT, ActInfos BLOB) \
            ENGINE MyISAM ",
				db_name.c_str(), db_t_stateinfo.c_str());    // using MyISAM as database engine, it's faster for writing but doesn't support transaction!
		if (mysql_query(db_con, tb_string))
		{
			fprintf(stderr, "Can't create stateinfo table, %s\n",
					mysql_error(db_con));
			return -1;
		}

		sprintf(tb_string,
				"CREATE TABLE IF NOT EXISTS %s.%s(Id MEDIUMINT NOT NULL AUTO_INCREMENT PRIMARY KEY, TimeStamp TIMESTAMP, DiscountRate FLOAT, Accuracy FLOAT, NumStates BIGINT, NumLinks BIGINT, LastState BIGINT, LastAction BIGINT) \
            ENGINE MyISAM ",
				db_name.c_str(), db_t_meminfo.c_str());
		if (mysql_query(db_con, tb_string))
		{
			fprintf(stderr, "Can't create meminfo table, %s\n",
					mysql_error(db_con));
			return -1;
		}
	}
	else
	{
		WARNNING("Unknown storage open flag: %d!\n", flag);
		return -1;
	}

	return 0;
}

/**
 * @brief Close the storage.
 */
void Mysql::close()
{
	if (db_con == NULL)
		return;
	else
	{
		mysql_close(db_con);
		mysql_library_end();
		db_con = NULL;
	}
}

/**
 * @brief Get the first state in storage.
 *
 * @return the first state
 */
Agent::State Mysql::firstState() const
{
	current_index = 0;
	return stateByIndex(current_index);
}

/**
 * @brief Get the next state in storage.
 *
 * @return the next state
 */
Agent::State Mysql::nextState() const
{
    ++current_index;
    return stateByIndex(current_index);
}

/**
 * @brief Get a state by its index in storage.
 *
 * @param [in] index the index in storage
 * @return the state at that index, or INVALID_STATE if out of boundary
 */
Agent::State Mysql::stateByIndex(unsigned long index) const
{
	char query_str[256];
	sprintf(query_str, "SELECT * FROM %s LIMIT %ld, 1", db_t_stateinfo.c_str(),
			index);

	if (mysql_query(db_con, query_str))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return Agent::INVALID_STATE;
	}

	MYSQL_RES *result = mysql_store_result(db_con);

	if (result == NULL)
	{
		dbgmoreprt("StateByIndex()", "result is  NULL!\n");
		return Agent::INVALID_STATE;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	unsigned long *lengths = mysql_fetch_lengths(result);

	if (lengths == NULL)
	{
		dbgmoreprt("StateByIndex()", "lengths is null\n");
		mysql_free_result(result);
		return Agent::INVALID_STATE;
	}
	Agent::State rs = atol(row[0]);

	mysql_free_result(result);    // free result
	return rs;
}

/**
 * @brief Get information of a specified state from storage.
 *
 * @param [in] st the requested state
 * @return address point of the state information
 */
struct State_Info_Header *Mysql::getStateInfo(Agent::State st) const
{
	if (st == Agent::INVALID_STATE)
	{
		dbgprt("Mysql GetStateInfo()", "invalid state value\n");
		return NULL;
	}

	// do mysql query
	char query_string[256];
	sprintf(query_string, "SELECT * FROM %s WHERE State=%" ST_FMT,
			db_t_stateinfo.c_str(), st);    // build mysql query

	if (mysql_query(db_con, query_string))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return NULL;
	}

	MYSQL_RES *result = mysql_store_result(db_con);

	if (result == NULL)
	{
		dbgmoreprt("Mysql GetStateInfo()", "result is NULL!\n");
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	int num_fields = mysql_num_fields(result);
	if (num_fields != 7)
	{
		dbgmoreprt("Mysql GetStateInfo()", "Field number doesn't match!\n");
		return NULL;
	}
	unsigned long *lengths = mysql_fetch_lengths(result);

	if (lengths == NULL)
	{
		dbgmoreprt("Mysql GetStateInfo()", "lengths is null\n");
		return NULL;
	}

	unsigned int sthd_size = atoi(row[5]);
	State_Info_Header *sthd = (State_Info_Header *) malloc(sthd_size);
	sthd->st = atol(row[0]);
	sthd->original_payoff = atof(row[1]);
	sthd->payoff = atof(row[2]);
	sthd->count = atol(row[3]);
	sthd->act_num = atoi(row[4]);
	sthd->size = sthd_size;

	unsigned char *stp = (unsigned char *) sthd;
	stp += sizeof(struct State_Info_Header);    // point to the first act
	unsigned int acif_size = sthd_size - sizeof(State_Info_Header);

	assert(acif_size == lengths[6]);    // check
	memcpy(stp, row[6], acif_size);    // copy action infos

	mysql_free_result(result);
	return sthd;
}

/**
 * @brief Check if a state exists in storage.
 *
 * @param [in] st the requested state
 * @return true|false
 */
bool Mysql::hasState(Agent::State st) const
{
	char query_string[256];
	sprintf(query_string, "SELECT * FROM %s WHERE State=%" ST_FMT,
			db_t_stateinfo.c_str(), st);

	if (mysql_query(db_con, query_string))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return false;
	}

	MYSQL_RES *result = mysql_store_result(db_con);

	int re;
	if (result->row_count == 0)    // 0 row found
		re = false;
	else
		re = true;

	mysql_free_result(result);    // free result
	return re;
}

/**
 * @brief Add a state to storage from the given information.
 *
 * When you add new states or links to the memory, make sure to update the memory information correspondingly.
 * Or the memory will stay inconsistent, and the loading will fail.
 * @param [in] sthd the state information
 */
void Mysql::addStateInfo(const struct State_Info_Header *sthd)
{
	unsigned long act_len = sthd->size - sizeof(State_Info_Header);

	char *stmt_buf = (char *) malloc(512 + 2 * act_len + 3);
	char *ptr;
	sprintf(stmt_buf,
			"INSERT INTO %s(State, OriPayoff, Payoff, Count, ActNum, Size, ActInfos) VALUES(%" ST_FMT ", %f, %f, %" UINT32_FMT ", %" UINT32_FMT ", %" UINT16_FMT ",'",
			db_t_stateinfo.c_str(), sthd->st, sthd->original_payoff,
			sthd->payoff, sthd->count, sthd->act_num, sthd->size);
	ptr = stmt_buf + strlen(stmt_buf);

	char *stp = (char *) sthd;
	stp += sizeof(struct State_Info_Header);    // point to the first act

	ptr += mysql_real_escape_string(db_con, ptr, stp, act_len);
	*ptr++ = '\'';
	*ptr++ = ')';

	if (mysql_real_query(db_con, (const char *) stmt_buf,
			(unsigned long) (ptr - stmt_buf)))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
	}

	free(stmt_buf);
	return;
}

/**
 * @brief Update a state in storage from the given information.
 *
 * When you add new states or links to the memory, make sure to update the memory information correspondingly.
 * Or the memory will stay unconsistent, and the loading will fail.
 * @param [in] sthd the state information
 */
void Mysql::updateStateInfo(const struct State_Info_Header *sthd)
{
	unsigned long act_len = sthd->size - sizeof(State_Info_Header);

	char *stmt_buf = (char *) malloc(512 + 2 * act_len + 3);
	char *ptr;

	sprintf(stmt_buf,
			"UPDATE %s SET OriPayoff=%f, Payoff=%f, Count=%" UINT32_FMT ", ActNum=%" UINT32_FMT ", Size=%" UINT16_FMT ", ActInfos='",
			db_t_stateinfo.c_str(), sthd->original_payoff, sthd->payoff,
			sthd->count, sthd->act_num, sthd->size);
	ptr = stmt_buf + strlen(stmt_buf);

	char *stp = (char *) sthd;
	stp += sizeof(struct State_Info_Header);    // point to the first act

	ptr += mysql_real_escape_string(db_con, ptr, stp, act_len);
	*ptr++ = '\'';

	int where_len = sprintf(ptr, " WHERE State=%" ST_FMT, sthd->st);
	ptr += where_len;

	if (mysql_real_query(db_con, (const char *) stmt_buf,
			(unsigned long) (ptr - stmt_buf)))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
	}

	free(stmt_buf);
	return;
}

/**
 * @brief Delete a state from storage.
 *
 * @param [in] st the state to be deleted
 * FIXME: need to handle the links with other states!
 */
void Mysql::deleteState(Agent::State st)
{
	char query_string[256];
	sprintf(query_string, "DELETE  FROM %s WHERE State=%" ST_FMT,
			db_t_stateinfo.c_str(), st);    // build delete query

	if (mysql_query(db_con, query_string))    // perform query
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return;
	}
	return;
}

/**
 * @brief Add the memory information to storage.
 *
 * @param [in] memif the memory information
 */
void Mysql::addMemoryInfo(const struct Memory_Info *memif)
{
	char query_str[1024];

	sprintf(query_str,
			"INSERT INTO %s(TimeStamp, DiscountRate, Accuracy, NumStates, NumLinks, LastState, LastAction) VALUES(NULL, %f, %f, %" UINT32_FMT ", %" UINT32_FMT ", %" ST_FMT ", %" ACT_FMT ")",
			db_t_meminfo.c_str(), memif->discount_rate, memif->accuracy,
			memif->state_num, memif->lk_num, memif->last_st, memif->last_act);    // build insert query

	int len = strlen(query_str);
	if (mysql_real_query(db_con, query_str, len))    // perform query
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return;
	}
	return;
}

/**
 * @brief Update the memory information in storage.
 *
 * @param [in] memif the memory information
 */
void Mysql::updateMemoryInfo(const struct Memory_Info *memif)
{
	char query_str[1024];

	sprintf(query_str,
			"UPDATE %s SET TimeStamp=NULL, DiscountRate=%f, Accuracy=%f, NumStates=%" UINT32_FMT ", NumLinks=%" UINT32_FMT ", LastState=%" ST_FMT ", LastAction=%" ACT_FMT " ORDER BY Id DESC LIMIT 1",
			db_t_meminfo.c_str(), memif->discount_rate, memif->accuracy,
			memif->state_num, memif->lk_num, memif->last_st, memif->last_act);

	int len = strlen(query_str);
	if (mysql_real_query(db_con, query_str, len))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return;
	}
	return;
}

/**
 * @brief Get the memory information from storage.
 *
 * @return address point of the memory information
 */
struct Memory_Info *Mysql::getMemoryInfo() const
{
	char query_str[256];
	sprintf(query_str, "SELECT * FROM %s ORDER BY Id DESC LIMIT 1",
			db_t_meminfo.c_str());    // select the lastest one

	if (mysql_query(db_con, query_str))
	{
		fprintf(stderr, "%s\n", mysql_error(db_con));
		return NULL;
	}

	MYSQL_RES *result = mysql_store_result(db_con);

	if (result == NULL)
	{
		dbgmoreprt("Mysql FetchMemoryInfo()", "result is NULL!\n");
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	unsigned long *lengths = mysql_fetch_lengths(result);

	if (lengths == NULL)
	{
		dbgmoreprt("Mysql FetchMemoryInfo()", "lengths is null\n");
		mysql_free_result(result);
		return NULL;
	}

	struct Memory_Info *memif = (struct Memory_Info *) malloc(
			sizeof(struct Memory_Info));
	dbgmoreprt("Mysql FetchMemoryInfo()", "%s, Memory TimeStamp: %s\n",
			db_name.c_str(), row[0]);
// fill in the memory struct, row[1] is Id
	memif->discount_rate = atof(row[2]);
	memif->accuracy = atof(row[3]);
	memif->state_num = atol(row[4]);
	memif->lk_num = atol(row[5]);
	memif->last_st = atol(row[6]);
	memif->last_act = atol(row[7]);

	mysql_free_result(result);    // free result

	return memif;
}

/**
 * @brief Get the memory name.
 *
 * @return the memory name
 */
std::string Mysql::getMemoryName() const
{
	return db_name;
}

}    // namespace gamcs
