
#include "usbdriver.h"

//win7及以前的版本使用
NTSTATUS CallUSBD(IN PDEVICE_EXTENSION pdx, IN PURB urb)
{
	PIRP irp;
	KEVENT event;
	NTSTATUS ntStatus;
	IO_STATUS_BLOCK ioStatus;
	PIO_STACK_LOCATION nextStack;

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
		pdx->NextStackDevice, NULL, 0, NULL, 0, TRUE, &event, &ioStatus);

	if (!irp)
	{
		MyDbgPrint(("build irp failed!!!"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	nextStack = IoGetNextIrpStackLocation(irp);
	ASSERT(nextStack != NULL);
	nextStack->Parameters.Others.Argument1 = urb;

	ntStatus = IoCallDriver(pdx->NextStackDevice, irp);
	if (ntStatus == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		ntStatus = ioStatus.Status;
	}

	MyDbgPrint(("leave CallUSBD"));
	return ntStatus;
}





//同步提交Urb请求的默认完成例程   激活事件
NTSTATUS UrbCompletionRoutine(PDEVICE_OBJECT /*DeviceObject*/,
	PIRP           Irp,
	PVOID          Context)
{
	PKEVENT kevent;

	kevent = (PKEVENT)Context;

	if (Irp->PendingReturned == TRUE)
	{
		KeSetEvent(kevent, IO_NO_INCREMENT, FALSE);
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Select-configuration request completed. \n"));


	return STATUS_MORE_PROCESSING_REQUIRED;
}

//win8及以后的版本使用     同步提交Urb请求到总线驱动
NTSTATUS SubmitUrbSync(PDEVICE_EXTENSION DeviceExtension,
	PURB Urb,
	BOOLEAN IsUrbBuildByNewMethod,
	PIO_COMPLETION_ROUTINE SyncCompletionRoutine)
{

	NTSTATUS  ntStatus;
	KEVENT    kEvent;
	// Allocate the IRP to send the buffer down the USB stack.
	// The IRP will be freed by IO manager.

	PIRP Irp = IoAllocateIrp((DeviceExtension->NextStackDevice->StackSize) + 1, TRUE);

	if (!Irp)
	{
		//Irp could not be allocated.
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	PIO_STACK_LOCATION nextStack;

	// Get the next stack location.
	nextStack = IoGetNextIrpStackLocation(Irp);

	// Set the major code.
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

	// Set the IOCTL code for URB submission.
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	// Attach the URB to this IRP.
	// The URB must be allocated by USBD_UrbAllocate, USBD_IsochUrbAllocate, 
	// USBD_SelectConfigUrbAllocateAndBuild, or USBD_SelectInterfaceUrbAllocateAndBuild.
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
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IoSetCompletionRoutineEx failed. \n"));
		goto Exit;
	}

	ntStatus = IoCallDriver(DeviceExtension->NextStackDevice, Irp);

	if (ntStatus == STATUS_PENDING)
	{
		KeWaitForSingleObject(&kEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}

	ntStatus = Irp->IoStatus.Status;

Exit:

	if (!NT_SUCCESS(ntStatus))
	{
		// We hit a failure condition, 
		// We will free the IRP

		IoFreeIrp(Irp);
		Irp = NULL;
	}


	return ntStatus;
}


//win8及以后的版本使用     异步提交Urb请求到总线驱动
NTSTATUS SubmitUrbASync(PDEVICE_EXTENSION DeviceExtension,
	PIRP Irp,
	PURB Urb,
	PIO_COMPLETION_ROUTINE CompletionRoutine,
	PVOID CompletionContext)
{
	// Completion routine is required if the URB is submitted asynchronously.
	// The caller's completion routine releases the IRP when it completes.


	NTSTATUS ntStatus = -1;

	PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);

	// Attach the URB to this IRP.
	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

	// Attach the URB to this IRP.
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	// Attach the URB to this IRP.
	(void)USBD_AssignUrbToIoStackLocation(DeviceExtension->UsbdHandle, nextStack, Urb);

	// Caller's completion routine will free the irp when it completes.
	ntStatus = IoSetCompletionRoutineEx(DeviceExtension->NextStackDevice,
		Irp,
		CompletionRoutine,
		CompletionContext,
		TRUE,
		TRUE,
		TRUE);

	if (!NT_SUCCESS(ntStatus))
	{
		goto Exit;
	}

	(void)IoCallDriver(DeviceExtension->NextStackDevice, Irp);

Exit:
	if (!NT_SUCCESS(ntStatus))
	{
		// We hit a failure condition, 
		// We will free the IRP

		IoFreeIrp(Irp);
		Irp = NULL;
	}

	return ntStatus;
}



