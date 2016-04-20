
#include "usbdriver.h"

extern GUID guid = { 0xae18aa60, 0x7f6a, 0x11d4, 0x97, 0xdd, 0x0, 0x1, 0x2, 0x29, 0xb9, 0x59 };
void DisplayProcessName()
{
	PEPROCESS hp = PsGetCurrentProcess();
	UCHAR* sname = PsGetProcessImageFileName(hp);
	MyDbgPrint(("当前进程:%s", sname));
}


#pragma INITCODE
extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING /*pRegistryPath*/)
{
	MyDbgPrint((" Enter DriverEntry\n"));
	DisplayProcessName();

	pDriverObject->DriverExtension->AddDevice = AddDevice;

	//即插即用消息
	pDriverObject->MajorFunction[IRP_MJ_PNP] = Pnp;
	//电源管理消息
	pDriverObject->MajorFunction[IRP_MJ_POWER] = DispatchRoutine;

	//createfile
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateFile;
	//closehandle
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = CleanUp;
	//deviceiocontrol
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIOControl;
	//readfile
	pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRoutine;
	//writefile
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchRoutine;

	pDriverObject->DriverUnload = Unload;


	MyDbgPrint((" Leave DriverEntry**************\n"));
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhyDeviceObject)
{
	PAGED_CODE();
	MyDbgPrint((" Enter addDevice"));
	DisplayProcessName();

	PDEVICE_OBJECT fdo;

	NTSTATUS status;

	status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_UNKNOWN, 0, false, &fdo);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	PDEVICE_EXTENSION pDevEx = reinterpret_cast<PDEVICE_EXTENSION>(fdo->DeviceExtension);
	pDevEx->fdo = fdo;
	pDevEx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, pPhyDeviceObject);
	PLIST_ENTRY entry = (PLIST_ENTRY)ExAllocatePool(PagedPool, sizeof(LIST_ENTRY));
	InitializeListHead(entry);
	pDevEx->pIrpListHead = entry;
	
	pDevEx->confDesc = NULL;
	pDevEx->deviceDesc = NULL;
	pDevEx->pipeInfos = NULL;
	pDevEx->UsbdHandle = NULL;
	status = USBD_CreateHandle(fdo, pDevEx->NextStackDevice, USBD_CLIENT_CONTRACT_VERSION_602, 1001, &pDevEx->UsbdHandle);
	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("USBD_CreateHandle failed!!!!"))
	}



	//创建设备接口
	status = IoRegisterDeviceInterface(pPhyDeviceObject, &guid, NULL, &pDevEx->ustrSymbolicName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	MyDbgPrint((" symbolicName %wZ", &pDevEx->ustrSymbolicName));

	status = IoSetDeviceInterfaceState(&pDevEx->ustrSymbolicName, TRUE);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	MyDbgPrint((" Leave addDevice**************"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS CreateFile(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter createfile"));
	DisplayProcessName();


	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	MyDbgPrint((" Leave createfile**************"));
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS CleanUp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter CleanUp"));
	DisplayProcessName();

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pFdo->DeviceExtension;
	PIRP_Entry pIrpEntry;
	while (!IsListEmpty(pDevExt->pIrpListHead))
	{
		PLIST_ENTRY pEntry = RemoveHeadList(pDevExt->pIrpListHead);
		pIrpEntry = CONTAINING_RECORD(pEntry,
			IRP_Entry,
			listEntry);
		pIrpEntry->pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrpEntry->pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrpEntry->pIrp, IO_NO_INCREMENT);

		ExFreePool(pIrpEntry);
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);


	MyDbgPrint((" Leave CleanUp**************"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS DeviceIOControl(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter deviceIOControl"));
	DisplayProcessName();

	static NTSTATUS(*fcntab[NUMBER_OF_ADAPT_IOCTLS])(PDEVICE_EXTENSION pdx, PIRP pIrp) =
	{
		 GetDriverVersion,
		 GetUsbDIVersion,
		 GetAltIntSetting,
		 SelIntface,
		 GetAddress,
		 GetNumEndpoints,
		 GetPwrState,
		 SetPwrState,
		 SendEP0Ctl,
		 SendNonEP0Ctl,
		 CyclePort,
		 ResetPipe,
		 ResetParentPort,
		 GetTransSize,
		 SetTransSize,
		 GetDiName,
		 GetFriendlyName,
		 AbortPipe,
		 SendNonEP0Direct,
		 GetSpeed,
		 GetCurrentFrame,
	};

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pFdo->DeviceExtension;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	for (ULONG i = 0; i < NUMBER_OF_ADAPT_IOCTLS; i++)
	{
		if (CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_ADAPT_INDEX + i, METHOD_BUFFERED, FILE_ANY_ACCESS) == code)
		{
			status = fcntab[i](pdx, pIrp);
			break;
		}
	}

	if (i == NUMBER_OF_ADAPT_IOCTLS)
	{
		//直接内存模式读写
		if (CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_ADAPT_INDEX + 18, METHOD_NEITHER, FILE_ANY_ACCESS) == code)
		{
			status = SendNonEP0Direct(pdx, pIrp);
		}
		else{
			MyDbgPrint(("didnot match method"));
			pIrp->IoStatus.Information = 0;
			pIrp->IoStatus.Status = status;
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			return  status;
		}
	}
	MyDbgPrint((" Leave deviceIOControl**************"));
	return status;
}

#pragma PAGEDCODE
NTSTATUS DispatchRoutine(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter dispatchRoutine"));
	DisplayProcessName();

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	MyDbgPrint((" Leave dispatchRoutine**************"));
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
void Unload(IN PDRIVER_OBJECT pDriverObject)
{
	PAGED_CODE();
	MyDbgPrint((" Enter unLoad"));
	DisplayProcessName();
	MyDbgPrint((" Leave unLoad**************"));
}

VOID OnCancelIrp(IN PDEVICE_OBJECT pDev, IN PIRP pIrp)
{
	MyDbgPrint(("enter cancelirp"));
	MyDbgPrint(("%s", KeGetCurrentIrql()));

	//当前irp已经出队列，准备由startio处理
	if (pIrp == pDev->CurrentIrp)
	{
		KIRQL oldirql = pIrp->CancelIrql;
		//pIrp->Cancel = true;
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
		IoStartNextPacket(pDev, TRUE);
		MyDbgPrint(("%s", KeGetCurrentIrql()));
		KeLowerIrql(oldirql);
	}
	else
	{
		KeRemoveEntryDeviceQueue(&pDev->DeviceQueue, &pIrp->Tail.Overlay.DeviceQueueEntry);
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
	}
	KIRQL irql;
	IoAcquireCancelSpinLock(&irql);
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	MyDbgPrint((" Leave cancelirp**************"));
}
