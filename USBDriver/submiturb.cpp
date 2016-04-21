
#include "usbdriver.h"



//同步提交Urb请求的默认完成例程   激活事件
NTSTATUS UrbSyncCompletionRoutine(PDEVICE_OBJECT /*DeviceObject*/,
	PIRP           Irp,
	PVOID          Context)
{
	PKEVENT kevent;
	kevent = (PKEVENT)Context;

	if (Irp->PendingReturned == TRUE)
	{
		KeSetEvent(kevent, IO_NO_INCREMENT, FALSE);
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}

//同步提交Urb请求到总线驱动
NTSTATUS SubmitUrbSync(PDEVICE_EXTENSION DeviceExtension,
	PURB Urb,
	BOOLEAN IsUrbBuildByNewMethod,
	PIO_COMPLETION_ROUTINE SyncCompletionRoutine)
{

	NTSTATUS  ntStatus;
	KEVENT    kEvent;

	PIRP Irp = IoAllocateIrp((DeviceExtension->NextStackDevice->StackSize) + 1, TRUE);
	if (!Irp)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	if (IsUrbBuildByNewMethod)
	{
		USBD_AssignUrbToIoStackLocation(DeviceExtension->UsbdHandle, nextStack, Urb);
	}
	else{
		nextStack->Parameters.Others.Argument1 = Urb;
	}

	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);
	ntStatus = IoSetCompletionRoutineEx(DeviceExtension->NextStackDevice,
		Irp,
		SyncCompletionRoutine,
		(PVOID)&kEvent,
		TRUE,
		TRUE,
		TRUE);

	if (!NT_SUCCESS(ntStatus))
	{
		goto Exit;
	}

	ntStatus = IoCallDriver(DeviceExtension->NextStackDevice, Irp);
	if (ntStatus == STATUS_PENDING)
	{
		KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, NULL);
	}
	ntStatus = Irp->IoStatus.Status;

Exit:
	if (!NT_SUCCESS(ntStatus))
	{
		IoFreeIrp(Irp);
		Irp = NULL;
	}
	return ntStatus;
}



//异步提交Urb请求的完成例程   复制内存
NTSTATUS UrbAsyncSyncCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	if (Irp->PendingReturned == TRUE)
	{
		IoMarkIrpPending(Irp);
	}

	MyDbgPrint(("enter urbcomp!!!!!!!!!!!!!status:%d", Irp->IoStatus.Status));
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
	if (NT_SUCCESS(Irp->IoStatus.Status))
		Irp->IoStatus.Information = outLen;

	return STATUS_SUCCESS;
}


//异步提交Urb请求到总线驱动
NTSTATUS SubmitUrbAsync(PDEVICE_EXTENSION DeviceExtension,
	PIRP Irp,
	PURB Urb,
	BOOLEAN IsUrbBuildByNewMethod,
	PIO_COMPLETION_ROUTINE CompletionRoutine)
{
	MyDbgPrint(("enter suburbAsync"));
	NTSTATUS ntStatus = -1;

	IoCopyCurrentIrpStackLocationToNext(Irp);
	PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	if (IsUrbBuildByNewMethod)
	{
		USBD_AssignUrbToIoStackLocation(DeviceExtension->UsbdHandle, nextStack, Urb);
	}
	else{
		nextStack->Parameters.Others.Argument1 = Urb;
	}

	// Caller's completion routine will free the irp when it completes.
	ntStatus = IoSetCompletionRoutineEx(DeviceExtension->NextStackDevice,
		Irp,
		CompletionRoutine,
		NULL,
		TRUE,
		TRUE,
		TRUE);

	if (!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	ntStatus = IoCallDriver(DeviceExtension->NextStackDevice, Irp);

	MyDbgPrint(("ntstatus:%d", ntStatus));
	return ntStatus;
}



