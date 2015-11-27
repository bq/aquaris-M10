#include <linux/module.h>
#include <linux/list.h>

#include "g3dbase_common_define.h"
#include "g3dkmd_define.h"


#include "tester.h"

#if defined(ENABLE_TESTER)

//struct list_head *g_ptcase_head = NULL;
LIST_HEAD(g_tcase_head);


int tester_add_case(tester_case_t *ptcase)
{
    INIT_LIST_HEAD(&ptcase->list);
    list_add(&ptcase->list, &g_tcase_head);

//    printk("[%s]: %p (%s, %s)\n", __FUNCTION__, ptcase, 
//            ptcase->group, ptcase->name);

    return 0;
}


static int _tester_case_setup(tester_case_t *ptcase)
{
    if (ptcase->status != ACTIVED) return -1;

    ptcase->isPass = TESTER_FALSE;

//    printk("[%s]: %p (%s, %s)\n", __FUNCTION__, ptcase, 
//            ptcase->group, ptcase->name);

    if (ptcase->setup) 
        ptcase->setup((void*)&ptcase->private_data);

    return 0;
}

static int _tester_case_teardown(tester_case_t *ptcase)
{
    if (ptcase->teardown) 
        ptcase->teardown(&ptcase->private_data);
    ptcase->status = TEARDOWNED;

//    printk("[%s]: %p (%s, %s)\n", __FUNCTION__, ptcase,
//            ptcase->group, ptcase->name);

    return 0;
}

static int _tester_case_is_pass(tester_case_t *ptcase, int isPass)
{
    if (ptcase->status != ACTIVED) return -1;

    ptcase->isPass = isPass;
    if (ptcase->pass) 
        ptcase->pass(&ptcase->private_data, isPass);

    return 0;
}

static int _tester_case_custom(tester_case_t *ptcase, void *pCData)
{
    if (ptcase->status != ACTIVED) return -1;

    if (ptcase->custom_func) 
        return ptcase->custom_func(&ptcase->private_data, pCData);

    return 0;
}


static int _tester_case_report(tester_case_t *ptcase, long *passCnt)
{
    if (ptcase->status == TEARDOWNED) {
        *passCnt = ptcase->passCnt;
        if (ptcase->report) {
            ptcase->report(&ptcase->private_data);
        }
    }

    return 0;
}


static tester_case_t * _getCaseInstance(const char *group, const char *name) {
    tester_case_t    *ptcase = NULL, *l_ptcase;
    static tester_case_t *s_last_match = NULL;

    // cache the last time matched.
    if (s_last_match
        && (s_last_match->status == ACTIVED)
        && (group == NULL || 0 == strncmp(s_last_match->group, group, MAX_TESTER_CASE_NAME))
        && ( name == NULL || 0 == strncmp(s_last_match->name, name, MAX_TESTER_CASE_NAME))
        ) {
//printk("[%s] %s %d (last_match)\n", __FUNCTION__, s_last_match->name, s_last_match->status);
        return s_last_match;
    }

    if (!list_empty(&g_tcase_head)) {
        list_for_each_entry(l_ptcase, &g_tcase_head, list) {
            if (l_ptcase->status == ACTIVED || l_ptcase->status == NOT_INITED) {
                ptcase = l_ptcase;
                break;
            }
        }
    }

    // if no match and find if any case need to test the more times.
    if (ptcase == NULL) {
        list_for_each_entry_reverse(l_ptcase, &g_tcase_head, list) {
            if (l_ptcase->testCnt != l_ptcase->MaxTestCnt) {
                ptcase = l_ptcase;
                l_ptcase->status = NOT_INITED; 
            }
        }
    }

    ptcase->status = ACTIVED;
    s_last_match = ptcase;
//    if (ptcase) printk("[%s] %s %d\n", __FUNCTION__, ptcase->name, ptcase->status);
//    else printk("[%s] ptcase is NULL\n", __FUNCTION__);
    return ptcase;
}


int  _tester_setup(const char* tcase_group_name)
{
    tester_case_t *ptcase = _getCaseInstance(tcase_group_name, NULL);

    if (ptcase) {
//printk("[%s] %s\n", __FUNCTION__, ptcase->name);
        return _tester_case_setup(ptcase);
    } 

    return -1;
}

int _tester_teardown(const char* tcase_group_name)
{
    tester_case_t *ptcase = _getCaseInstance(tcase_group_name, NULL);

    if (ptcase) {
        ptcase->status = TEARDOWNED;
        ptcase->testCnt++; 
        if (ptcase->isPass == TESTER_PASS)
            ptcase->passCnt++;

//printk("[%s] %s %d\n", __FUNCTION__, ptcase->name, ptcase->status);

        return _tester_case_teardown(ptcase);
    }

    return -1;
}

int _tester_is_pass(const char * group, const char* name, int isPass)
{
    tester_case_t * ptcase = _getCaseInstance(group, name);

    if (ptcase && 
        (0 == strncmp(ptcase->group, group, MAX_TESTER_CASE_NAME)) &&
        (0 == strncmp(ptcase->name, name, MAX_TESTER_CASE_NAME))) {
//printk("[%s] %s\n", __FUNCTION__, ptcase->name);
        ptcase->isPass = isPass;
        return _tester_case_is_pass(ptcase, isPass);
    }

    return -1;
}

int _tester_custom(const char *group, const char *name,void* customData)
{
    tester_case_t * ptcase = _getCaseInstance(group, name);

    if (ptcase && 
        (0 == strncmp(ptcase->group, group, MAX_TESTER_CASE_NAME)) &&
        (0 == strncmp(ptcase->name, name, MAX_TESTER_CASE_NAME))) {
//printk("[%s] %s\n", __FUNCTION__, ptcase->name);
        return _tester_case_custom(ptcase, customData);
    }

    return -1;
}

int _tester_register_case( tester_case_t *ptcase)
{
    if (ptcase) {
        printk("test case: (%s,%s) is registered.\n",
            ptcase->group, ptcase->name);
        tester_add_case(ptcase);
    }

    return 0;
}


int _tester_reports( const char *group, const char *name)
{
    tester_case_t *ptcase;
    long caseCnt = 0, passCnt = 0;


    printk( "=============== Tester Report ===============\n");
    list_for_each_entry(ptcase, &g_tcase_head, list) {
        if (ptcase 
            && (group == NULL || 0 == strncmp(ptcase->group, group, MAX_TESTER_CASE_NAME))
            && ( name == NULL || 0 == strncmp(ptcase->name,  name,  MAX_TESTER_CASE_NAME))) {
            long casePassCnt=0;
            // report
            _tester_case_report(ptcase, &casePassCnt);
            caseCnt++;
            if(casePassCnt!=0) passCnt++;
        }
    }
    printk( "---------------------------------------------\n");
    printk( " Total pass/case:  %ld/%ld\n", passCnt, caseCnt);
    printk( "=============================================\n");

    return 0;
}

#endif //defined(ENABLE_TESTER)
