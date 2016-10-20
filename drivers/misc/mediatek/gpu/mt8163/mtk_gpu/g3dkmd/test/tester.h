/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _TESTER__H_
#define _TESTER__H_


#define TESTER_TRUE  1
#define TESTER_FALSE 0

#define TESTER_PASS  TESTER_TRUE
#define TESTER_FAIL  TESTER_FALSE

#define MAX_TESTER_CASE_NAME  64




#if !defined(ENABLE_TESTER)

#define tester_setup(tcase)
#define tester_teardown(tcase)
#define tester_is_pass(group, name, isPass)
#define tester_reports(group, name)
#define tester_custom(group, name, cdata)
#define tester_register_testcase(icase)

#else


enum tester_case_status_t {
	NOT_INITED = 0,
	ACTIVED,
	TEARDOWNED,
	DONE
};

struct tester_case_t {
	enum tester_case_status_t status;
	char group[MAX_TESTER_CASE_NAME];
	char name[MAX_TESTER_CASE_NAME];
	int isPass;
	long testCnt;		/* the times to test, -1 if never stop test. */
	long MaxTestCnt;
	long passCnt;
	void *private_data;
	void (*setup)(void *private_data);
	void (*teardown)(void *private_data);
	void (*pass)(void *private_data, int isPass);
	int (*custom_func)(void *private_data, void *custom_data);
	void (*report)(void *private_data);

	struct list_head list;
};



int tester_add_case(struct tester_case_t *ptcase);



int _tester_setup(const char *tcase_name);
int _tester_teardown(const char *tcase_name);
int _tester_is_pass(const char *gruop, const char *tcase_name, int isPass);
int _tester_reports(const char *group, const char *name);
int _tester_custom(const char *group, const char *name, void *customData);

int _tester_register_case(struct tester_case_t *tcase);

#define tester_setup(tcase)                _tester_setup(tcase)
#define tester_teardown(tcase)             _tester_teardown(tcase)
#define tester_is_pass(group, name, isPass) _tester_is_pass(group, name, isPass)
#define tester_reports(group, name)        _tester_reports(group, name)
#define tester_custom(group, name, cdata)  _tester_custom(group, name, cdata)

#define tester_register_testcase(icase) _tester_register_case(&icase)


#define DECLARE_TEST_CASE(var, igroup, iname, setup_func, teardown_func,\
				ispass_func, iCustom_func, report_func, iMaxTestCnt)\
struct tester_case_t var = {\
	.group      = igroup,\
	.name       = iname,\
	.setup      = setup_func,\
	.teardown   = teardown_func,\
	.pass       = ispass_func,\
	.report     = report_func,\
	.MaxTestCnt = iMaxTestCnt,\
	.isPass     = TESTER_FALSE,\
	.custom_func = iCustom_func,\
	.status     = NOT_INITED,\
	.testCnt    = 0,\
	.passCnt    = 0,\
}


#define REGISTER_TEST_CASE(tcase) \
				/* static const int _tester_case__##tcase = _tester_register_case(&tcase) */



#endif /* !defined(ENABLE_TESTER) */

#endif
