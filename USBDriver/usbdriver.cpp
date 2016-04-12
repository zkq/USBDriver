
#include "usbdriver.h"

extern GUID guid = { 0xae18aa60, 0x7f6a, 0x11d4, 0x97, 0xdd, 0x0, 0x1, 0x2, 0x29, 0xb9, 0x59 };
void DisplayProcessName()
{
	PEPROCESS hp = PsGetCurrentProcess();
	UCHAR* sname = PsGetProcessImageFileName(hp);
	MyDbgPrint(("��ǰ����:%s", sname));
}


#pragma INITCODE
extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING /*pRegistryPath*/)
{
	MyDbgPrint((" Enter DriverEntry\n"));
	DisplayProcessName();

	pDriverObject->DriverExtension->AddDevice = AddDevice;

	//���弴����Ϣ
	pDriverObject->MajorFunction[IRP_MJ_PNP] = Pnp;
	//��Դ������Ϣ
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

	pDriverObject->DeviceObject;
	pDriverObject->DriverExtension->AddDevice;
	pDriverObject->DriverExtension->Count;
	pDriverObject->DriverExtension->DriverObject;
	pDriverObject->DriverExtension->ServiceKeyName;
	pDriverObject->DriverInit;
	pDriverObject->DriverName;
	pDriverObject->DriverSection;
	pDriverObject->DriverSize;
	pDriverObject->DriverStart;
	pDriverObject->DriverStartIo = StartIO;
	pDriverObject->DriverUnload;
	pDriverObject->FastIoDispatch;
	pDriverObject->Flags;
	pDriverObject->HardwareDatabase;
	pDriverObject->MajorFunction;
	pDriverObject->Size;
	pDriverObject->Type;


	MyDbgPrint((" Leave DriverEntry**************\n"));
	return STATUS_SUCCESS;
}

#pragma LOCKEDCODE
VOID StartIO(IN PDEVICE_OBJECT pDev, IN PIRP pIrp)
{
	MyDbgPrint(("enter startio***********"));

	KIRQL oldIrql;
	IoAcquireCancelSpinLock(&oldIrql);
	if (pIrp != pDev->CurrentIrp || pIrp->Cancel)
	{
		IoReleaseCancelSpinLock(oldIrql);
		MyDbgPrint(("leave startio false"));
		return;
	}
	else{
		IoSetCancelRoutine(pIrp, NULL);
		IoReleaseCancelSpinLock(oldIrql);
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	IoStartNextPacket(pDev, TRUE);
	MyDbgPrint(("leave startio true"));
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



	//�����豸�ӿ�
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
		 GetUSBDIVersion,
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
		if (CTL_CODE(FILE_DEVICE_UNKNOWN, i, METHOD_BUFFERED, FILE_ANY_ACCESS) == code)
		{
			status = fcntab[i](pdx, pIrp);
			break;
		}
	}

	if (i == NUMBER_OF_ADAPT_IOCTLS)
	{
		MyDbgPrint(("didnot match method"));
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return  status;
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

VOID CancelIrp(IN PDEVICE_OBJECT pDev, IN PIRP pIrp)
{
	MyDbgPrint(("enter cancelirp"));
	MyDbgPrint(("%s", KeGetCurrentIrql()));

	//��ǰirp�Ѿ������У�׼����startio����
	if (pIrp == pDev->CurrentIrp)
	{

		pIrp->Cancel = true;
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
		IoStartNextPacket(pDev, TRUE);
		MyDbgPrint(("%s", KeGetCurrentIrql()));
		//KeLowerIrql()
	}
	else
	{
		KeRemoveEntryDeviceQueue(&pDev->DeviceQueue, &pIrp->Tail.Overlay.DeviceQueueEntry);
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
	}

	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	MyDbgPrint((" Leave cancelirp**************"));
}
