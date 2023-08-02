#include <stdio.h>

typedef struct tag_TPara {
    short item;
    struct tag_TPara *pNext;
} TPara;

typedef struct _TControl {
    TPara *globalParaList;
} TControl;

typedef struct tag_TType {
    struct tag_TType *pLeft, *pRight;
    unsigned char CmpntAttr;
    unsigned short GlobalSwitch;
    TControl *pControl;
} TType;

static unsigned char GetGlobalSwitch(TType *pType, unsigned short *pGlobalSwitch)
{
    unsigned short GlobalSwitch;
    TPara *pPara = ((void *)0);

    if (!pGlobalSwitch || !pType) {
        return 0;
    }
    *pGlobalSwitch = 0;

    GlobalSwitch = (unsigned short)pType->GlobalSwitch;

    if (pType->pControl != ((void *)0)) {
        for (pPara = pType->pControl->globalParaList; ((void *)0) != pPara; pPara = pPara->pNext) {
            if (0==((unsigned char)(pPara->item))) {
                GlobalSwitch |= 0x70;
            } else {
                GlobalSwitch |= 0x80;
            }
        }
    }

    *pGlobalSwitch = GlobalSwitch;
    return 1;
}

static void SetGlobalSwitch(TType *pType, unsigned short GlobalSwitch)
{
    pType->GlobalSwitch = GlobalSwitch;
    return;
}

static void GlobalSwitchCover(TType *pF)
{
    TType *p = ((void *)0);
    unsigned short GlobalSwitch;

    if (GetGlobalSwitch(pF, &GlobalSwitch) == 0) {
        return;
    }
    SetGlobalSwitch(pF, GlobalSwitch);

    for (p = pF->pLeft; ((void *)0) != p; p = p->pRight) {
        if (p->CmpntAttr != 0) {
            SetGlobalSwitch(p, GlobalSwitch);
        }
        GlobalSwitchCover(p);
    }

    return;
}

static void InitGlobalSwitch(void)
{
    TType tType;
    TControl tControl;
    TPara tPara;

    tType.GlobalSwitch = 1;
    tType.CmpntAttr = 10;
    tType.pLeft = 0;
    tType.pRight = 0;

    tPara.item = 10;
    tPara.pNext = 0;

    tControl.globalParaList = &tPara;
    tType.pControl = &tControl;

    GlobalSwitchCover(&tType);
    printf("%x\n", tType.GlobalSwitch);
    return;
}

int main()
{
    InitGlobalSwitch();
    return 0;
}
