/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2019/11/9.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * protect.c包含与Intel处理器保护模式相关的例程。
 */

#include "kernel.h"
#include "protect.h"
#include "process.h"

#if _WORD_SIZE == 4
#define INT_GATE_TYPE	(INT_286_GATE | DESC_386_BIT)
#define TSS_TYPE	(AVL_286_TSS  | DESC_386_BIT)
#else
#define INT_GATE_TYPE	INT_286_GATE
#define TSS_TYPE	AVL_286_TSS
#endif

PUBLIC SegDescriptor gdt[GDT_SIZE];     /* 全局描述符表 */
PRIVATE Gate idt[IDT_SIZE];             /* 中断描述符表 */
PUBLIC Tss tss;                         /* 用0初始化 */

FORWARD _PROTOTYPE(void init_gate, (u8_t vector, u8_t desc_type, int_handler_t handler, u8_t privilege) );
/*=========================================================================*
 *				protect_init				   *
 *				保护模式初始化
 *=========================================================================*/
PUBLIC void protect_init(void)
{
    /* 建立CPU的保护机制和中断表
     * 每个进程在进程表中LDT的空间都分配，每个包含有两个描述符，
     * 分别是代码段和数据段描述符-这个讨论的段由硬件定义。这些段
     * 与操作系统中的段不同，操作系统中的段将硬件定义的数据段进一
     * 步分为数据段和堆栈段。
     */

    /* 首先，将LOADER中的GDT复制到新的GDT中 */
    phys_copy((phys_bytes) (*((u32_t *) (&gdt_ptr[2]))),      /* LOADER中的旧GDT基地址 */
            (phys_bytes) &gdt,                                        /* 移动到新的GDT */
            *((u16_t *)(&gdt_ptr[0])) + 1);                    /* 移动字节量：GDT界限 + 1 */

    /* gdt_ptr[6] 共 6 个字节: 0~15:Limit  16~47:Base。用作 sgdt 以及 lgdt 的参数。 */
    u16_t *p_gdtLimit	= (u16_t *)(&gdt_ptr[0]);
    u32_t *p_gdtBase 	= (u32_t *)(&gdt_ptr[2]);
    *p_gdtLimit = GDT_SIZE * DESCRIPTOR_SIZE - 1;
    *p_gdtBase	= (u32_t)&gdt;

    /* idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt 以及 lidt 的参数。 */
    u16_t *p_idtLimit = (u16_t*)(&idt_ptr[0]);
    u32_t *p_idtBase  = (u32_t*)(&idt_ptr[2]);
    *p_idtLimit = IDT_SIZE * sizeof(Gate) - 1;
    *p_idtBase  = (u32_t)&idt;

    /* =================  全部初始化成中断门(没有陷阱门) =================  */
    /* 异常 */
    init_gate(INT_VECTOR_DIVIDE,	DA_386IGate, divide_error,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_DEBUG,		DA_386IGate, single_step_exception,	KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_NMI,		DA_386IGate, nmi,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_BREAKPOINT,	DA_386IGate, breakpoint_exception,	USER_PRIVILEGE);
    init_gate(INT_VECTOR_OVERFLOW,	DA_386IGate, overflow,			USER_PRIVILEGE);
    init_gate(INT_VECTOR_BOUNDS,	DA_386IGate, bounds_check,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_INVAL_OP,	DA_386IGate, inval_opcode,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_COPROC_NOT,	DA_386IGate, copr_not_available,	KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_DOUBLE_FAULT,	DA_386IGate, double_fault,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_COPROC_SEG,	DA_386IGate, copr_seg_overrun,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_INVAL_TSS,	DA_386IGate, inval_tss,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_SEG_NOT,	DA_386IGate, segment_not_present,	KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_STACK_FAULT,	DA_386IGate, stack_exception,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_PROTECTION,	DA_386IGate, general_protection,	KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_PAGE_FAULT,	DA_386IGate, page_fault,		KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_COPROC_ERR,	DA_386IGate, copr_error,		KERNEL_PRIVILEGE);
    /* 硬件中断 */
    init_gate(INT_VECTOR_IRQ0 + 0,	DA_386IGate, hwint00,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 1,	DA_386IGate, hwint01,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 2,	DA_386IGate, hwint02,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 3,	DA_386IGate, hwint03,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 4,	DA_386IGate, hwint04,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 5,	DA_386IGate, hwint05,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 6,	DA_386IGate, hwint06,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ0 + 7,	DA_386IGate, hwint07,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 0,	DA_386IGate, hwint08,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 1,	DA_386IGate, hwint09,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 2,	DA_386IGate, hwint10,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 3,	DA_386IGate, hwint11,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 4,	DA_386IGate, hwint12,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 5,	DA_386IGate, hwint13,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 6,	DA_386IGate, hwint14,			KERNEL_PRIVILEGE);
    init_gate(INT_VECTOR_IRQ8 + 7,	DA_386IGate, hwint15,			KERNEL_PRIVILEGE);
    /* 软件中断 */
#if _WORD_SIZE == 2     /* 286系统调用中断门 */
    init_gate(INT_VECTOR_SYS_CALL, 	DA_386IGate, flyanx_286_syscall, 	USER_PRIVILEGE);
#else                   /* 386系统调用中断门 */
    init_gate(INT_VECTOR_SYS_CALL, 	DA_386IGate, flyanx_386_syscall, 	USER_PRIVILEGE);
#endif
    init_gate(INT_VECTOR_LEVEL0, 	DA_386IGate, level0_call, 	TASK_PRIVILEGE);    /* 系统任务提权调用 */

    /* 初始化任务状态段TSS，并为处理器寄存器和其他任务切换时应保存的信息提供空间。
     * 我们只使用了某些域的信息，这些域定义了当发生中断时在何处建立新堆栈。
     * 下面init_seg_desc的调用保证它可以用GDT进行定位。
     */
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    init_seg_desc(&gdt[TSS_INDEX],
                vir2phys(&tss),
                sizeof(tss) - 1,
                DA_386TSS);
    /* 完成主要的TSS的构建。 */
    tss.iobase = sizeof(tss);	/* 空的I/O权限图 */

    /* 在GDT中为进程表中的LDT构建本地描述符。
     * LDT在编译时将分配给进程，并在初始化或更改进程映射时再次初始化。
     */
    int i;
    for(i = 0; i < NR_TASKS + NR_PROCS; i++){
        memset(&process[i], 0, sizeof(Process));
        process[i].ldt_sel = SELECTOR_LDT_FIRST + (i * DESCRIPTOR_SIZE);
        init_seg_desc(&gdt[LDT_FIRST_INDEX + i],
                vir2phys(process[i].ldt),
                LDT_SIZE * DESCRIPTOR_SIZE - 1,
                DA_LDT);
    }

    /* ================= 至此，保护模式所需的基本信息已经构建完成 ================= */
}

/*=========================================================================*
 *				init_seg_desc				   *
 *				初始化段描述符
 *=========================================================================*/
PUBLIC void init_seg_desc(p_desc, base, limit, attribute)
SegDescriptor *p_desc;
phys_bytes base;
phys_bytes limit;
u16_t attribute;
{
    /* 初始化一个数据段描述符 */
    p_desc->limit_low	= limit & 0x0FFFF;         /* 段界限 1		(2 字节) */
    p_desc->base_low	= base & 0x0FFFF;          /* 段基址 1		(2 字节) */
    p_desc->base_middle	= (base >> 16) & 0x0FF;     /* 段基址 2		(1 字节) */
    p_desc->access		= attribute & 0xFF;         /* 属性 1 */
    p_desc->granularity = ((limit >> 16) & 0x0F) |  /* 段界限 2 + 属性 2 */
                            ((attribute >> 8) & 0xF0);
    p_desc->base_high	= (base >> 24) & 0x0FF;     /* 段基址 3		(1 字节) */
}

/*=========================================================================*
 *				init_gate				   *
 *				初始化一个 386 中断门
 *=========================================================================*/
PRIVATE void init_gate(vector, desc_type, handler, privilege)
u8_t vector;
u8_t desc_type;
int_handler_t  handler;
u8_t privilege;
{
    // 得到中断向量对应的门结构
    Gate* p_gate = &idt[vector];
    // 取得处理函数的基地址
    u32_t base_addr = (u32_t)handler;
    // 一一赋值
    p_gate->offset_low = base_addr & 0xFFFF;
    p_gate->selector = SELECTOR_KERNEL_CS;
    p_gate->dcount = 0;
    p_gate->attr = desc_type | (privilege << 5);
#if _WORD_SIZE == 4
    p_gate->offset_high = (base_addr >> 16) & 0xFFFF;
#endif
}

/*=========================================================================*
 *				seg2phy				   *
 *				由段名求其在内存中的绝对地址
 *=========================================================================*/
PUBLIC phys_bytes seg2phys(U16_t seg)
{
    SegDescriptor* p_dest = &gdt[seg >> 3];
    return (p_dest->base_high << 24 | p_dest->base_middle << 16 | p_dest->base_low);
}

/*=========================================================================*
 *				phys2seg				   *
 *			通过段基址得到对应的段描述符
 *=========================================================================*/
PUBLIC void phys2seg(
        vir_bytes *seg,
        vir_bytes *off,
        phys_bytes phys
){
#if _WORD_SIZE == 4
    *seg = SELECTOR_KERNEL_DS;
    *off = phys;
#endif
}



