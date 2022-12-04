#!/bin/bash
ORIGIN_WORK_DIR=$PWD

WORK_DIR=$(cd `dirname $0`; pwd)
cd $WORK_DIR || exit

MNTPOINT='./mnt'
PROJECT_NAME="SAMPLE_PROJECT_NAME"
ALL_POINTS=28
POINTS=0

function pass() {
    RES=$1
    POINTS=$(($POINTS+1))
    echo -e "\033[32mpass: ${RES}\033[0m"
}

function fail() {
    RES=$1
    echo -e "\033[31mfail: ${RES}\033[0m"
}

function test_mount() {
    TEST_CASE=$1
    echo ">>>>>>>>>>>>>>>>>>>> TEST_MOUNT"
    ../build/${PROJECT_NAME} --device="$HOME"/ddriver ${MNTPOINT}
    if [ $? -ne 0 ]; then
        fail $TEST_CASE
        exit 1
    fi
    pass $TEST_CASE
    
    echo "<<<<<<<<<<<<<<<<<<<<"
}

function core_tester() {
    CMD=$1
    PARAM=$2
    echo "TEST: "$CMD $PARAM
    $CMD $PARAM
    if [ $? -ne 0 ]; then
        fail $CMD $PARAM
        exit 1
    fi
    pass "-> $CMD $PARAM"
}

function test_mkdir() {
    TEST_CASE=$1
    echo ">>>>>>>>>>>>>>>>>>>> TEST_MKDIR"
    
    core_tester mkdir ${MNTPOINT}/dir0
    core_tester mkdir ${MNTPOINT}/dir0/dir0
    core_tester mkdir ${MNTPOINT}/dir0/dir0/dir0
    core_tester mkdir ${MNTPOINT}/dir1

    pass $TEST_CASE
    
    echo "<<<<<<<<<<<<<<<<<<<<"
}

function test_touch() {
    TEST_CASE=$1
    echo ">>>>>>>>>>>>>>>>>>>> TEST_TOUCH"
    
    core_tester touch ${MNTPOINT}/file0;
    core_tester touch ${MNTPOINT}/dir0/file0;
    core_tester touch ${MNTPOINT}/dir0/dir0/file0;
    core_tester touch ${MNTPOINT}/dir0/dir0/dir0/file0;
    core_tester touch ${MNTPOINT}/dir1/file0;

    pass $TEST_CASE
    
    echo "<<<<<<<<<<<<<<<<<<<<"
}

function test_ls() {
    TEST_CASE=$1
    echo ">>>>>>>>>>>>>>>>>>>> TEST_LS"
    
    core_tester ls ${MNTPOINT}/;
    core_tester ls ${MNTPOINT}/dir0;
    core_tester ls ${MNTPOINT}/dir0/dir0;
    core_tester ls ${MNTPOINT}/dir0/dir0/dir0;
    core_tester ls ${MNTPOINT}/dir1;

    pass $TEST_CASE
    
    echo "<<<<<<<<<<<<<<<<<<<<"
}

function test_cp() {
    TEST_CASE=$1
    echo ">>>>>>>>>>>>>>>>>>>> TEST_CP"
    
    cp ${MNTPOINT}/file0 ${MNTPOINT}/file1 
    if [ $? -ne 0 ]; then
        fail $TEST_CASE
        exit 1
    fi

    pass $TEST_CASE
    
    echo "<<<<<<<<<<<<<<<<<<<<"
}

function test_remount() {
    TEST_CASE=$1
    echo ">>>>>>>>>>>>>>>>>>>> TEST_REMOUNT"
    
    fusermount -u ${MNTPOINT}
    if [ $? -ne 0 ]; then
        fail "umount"
        exit 1
    fi
    pass "-> fusermount -u ${MNTPOINT}"

    ../build/${PROJECT_NAME} --device="$HOME"/ddriver ${MNTPOINT}
    if [ $? -ne 0 ]; then
        fail "remount"
        exit 1
    fi
    pass "-> ../build/${PROJECT_NAME} --device=""$HOME""/ddriver ${MNTPOINT}"
    
    core_tester ls ${MNTPOINT}/;
    core_tester ls ${MNTPOINT}/dir0;
    core_tester ls ${MNTPOINT}/dir0/dir0;
    core_tester ls ${MNTPOINT}/dir0/dir0/dir0;
    core_tester ls ${MNTPOINT}/dir1;

    sleep 1
    
    fusermount -u ${MNTPOINT}
    if [ $? -ne 0 ]; then
        fail "umount finally"
        exit 1
    fi
    pass "-> fusermount -u ${MNTPOINT}"

    pass $TEST_CASE

    echo "<<<<<<<<<<<<<<<<<<<<"
}


function test_main() {
    # ddriver -r
    rm ~/ddriver -f
    touch ~/ddriver 
    
    test_mount "[all-the-mount-test]"
    echo ""
    test_mkdir "[all-the-mkdir-test]"
    echo ""
    test_touch "[all-the-mkdir-test]"
    echo ""
    test_cp "[all-the-cp-test]"
    echo ""
    test_ls "[all-the-ls-test]"
    echo ""
    test_remount "[all-the-remount-test]"
    echo ""

    if [ $POINTS -eq $ALL_POINTS ]; then
        pass "恭喜你，通过所有测试 ($ALL_POINTS/$ALL_POINTS)"
    else 
        fail "再接再厉! ($POINTS/$ALL_POINTS)"
    fi
}

test_main
cd $ORIGIN_WORK_DIR