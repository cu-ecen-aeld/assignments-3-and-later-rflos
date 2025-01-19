#faulty-oops analysis

##oops messages
```
#echo "hello" > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045, ISS2 = 0x00000000
  CM = 0, WnR = 1, TnD = 0, TagAccess = 0
  GCS = 0, Overlay = 0, DirtyBit = 0, Xs = 0
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b8b000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 159 Comm: sh Tainted: G           O       6.6.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x8/0x10 [faulty]
lr : vfs_write+0xb8/0x384
sp : ffffffc080dabd20
x29: ffffffc080dabd20 x28: ffffff8001b1cf80 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040000000 x22: 0000000000000006 x21: 000000558f9a2a70
x20: 000000558f9a2a70 x19: ffffff8001b33200 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000400100 x4 : ffffffc078ba9000 x3 : ffffffc080dabdf0
x2 : 0000000000000006 x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x8/0x10 [faulty]
 ksys_write+0x68/0xf4
 __arm64_sys_write+0x1c/0x28
 invoke_syscall+0x54/0x124
 el0_svc_common.constprop.0+0x40/0xe0
 do_el0_svc+0x1c/0x28
 el0_svc+0x40/0xf4
 el0t_64_sync_handler+0xc0/0xc4
 el0t_64_sync+0x190/0x194
Code: ???????? ???????? d2800001 d2800000 (b900003f) 
---[ end trace 0000000000000000 ]---
```

##analysis
given the message
> Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
and
>  faulty_write+0x8/0x10 [faulty]
The faulty condition might be caused by attempting to write a NULL pointer

Based on the source code
```
ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,
		loff_t *pos)
{
	/* make a simple fault by dereferencing a NULL pointer */
	*(int *)0 = 0;
	return 0;
}
```
There is a line trying to reference the NULL pointer and thus causes the failure

##conclusion
Based on the oops messages and faulty.c, the failure is caused by trying the write a NULL pointer.
