#include "vm.h"
#include <stdio.h>

VMResult VM_Execute(VMContext* ctx) {
    printf("VM_Execute: Executing instructions\n");
    VMResult res = {VM_STATUS_SUCCESS, 0, "Success"};
    return res;
}
