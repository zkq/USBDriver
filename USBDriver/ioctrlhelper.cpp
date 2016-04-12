

#include "usbdriver.h"

NTSTATUS GetDeviceDesc(PDEVICE_EXTENSION pdx, bool refresh)
{
	NTSTATUS status;
	//已经获取而且不用刷新
	if (pdx->deviceDesc != NULL && !refresh)
	{
		MyDbgPrint(("device desc old"));
		status = STATUS_SUCCESS;
		return status;
	}
	if (pdx->deviceDesc)
	{
		ExFreePool(pdx->deviceDesc);
	}

	pdx->deviceDesc = (PUSB_DEVICE_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeof(USB_DEVICE_DESCRIPTOR));
	if (!pdx->deviceDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	UsbBuildGetDescriptorRequest(urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, pdx->deviceDesc, NULL, sizeof(USB_DEVICE_DESCRIPTOR), NULL);
	status = SubmitUrbSync(pdx, urb);
	USBD_UrbFree(pdx->UsbdHandle, urb);
	return status;
}


NTSTATUS GetConfDesc(PDEVICE_EXTENSION pdx, bool refresh)
{
	NTSTATUS status;
	//已经获取而且不用刷新
	if (pdx->confDesc != NULL && !refresh)
	{
		MyDbgPrint(("device desc old"));
		status = STATUS_SUCCESS;
		return status;
	}
	if (pdx->confDesc)
	{
		ExFreePool(pdx->confDesc);
	}

	//第一步获取大小
	pdx->confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeof(USB_CONFIGURATION_DESCRIPTOR));
	if (!pdx->confDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	UsbBuildGetDescriptorRequest(urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pdx->confDesc, NULL, sizeof(USB_CONFIGURATION_DESCRIPTOR), NULL);
	status = SubmitUrbSync(pdx, urb);
	USBD_UrbFree(pdx->UsbdHandle, urb);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	//第二步获取完整的
	ULONG size = pdx->confDesc->wTotalLength;
	ExFreePool(pdx->confDesc);
	pdx->confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, size);
	if (!pdx->confDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	PURB urb2;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb2);
	UsbBuildGetDescriptorRequest(urb2, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pdx->confDesc, NULL, size, NULL);
	status = SubmitUrbSync(pdx, urb2);
	USBD_UrbFree(pdx->UsbdHandle, urb2);
	return status;
}


NTSTATUS SelectConfiguration(PDEVICE_EXTENSION pdx)
{
	PURB urb = NULL;

	NTSTATUS  status;

	PUSBD_INTERFACE_LIST_ENTRY   interfaceList = NULL;
	PUSB_INTERFACE_DESCRIPTOR    interfaceDescriptor = NULL;
	PUSBD_INTERFACE_INFORMATION  Interface = NULL;

	ULONG                        interfaceIndex;

	if (!pdx->confDesc)
	{
		status = GetConfDesc(pdx);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
	}



	// Allocate an array for the list of interfaces
	// The number of elements must be one more than number of interfaces.
	interfaceList = (PUSBD_INTERFACE_LIST_ENTRY)ExAllocatePool(
		NonPagedPool,
		sizeof(USBD_INTERFACE_LIST_ENTRY) *
		(pdx->confDesc->bNumInterfaces + 1));

	if (!interfaceList)
	{
		//Failed to allocate memory
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	// Initialize the array by setting all members to NULL.
	RtlZeroMemory(interfaceList, sizeof(
		USBD_INTERFACE_LIST_ENTRY) *
		(pdx->confDesc->bNumInterfaces + 1));
	// Enumerate interfaces in the configuration.
	PUCHAR StartPosition = (PUCHAR)pdx->confDesc;
	for (interfaceIndex = 0;
		interfaceIndex < pdx->confDesc->bNumInterfaces;
		interfaceIndex++)
	{
		if (interfaceIndex == 0)
			interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)(StartPosition + pdx->confDesc->bLength);
		else
			interfaceDescriptor = NULL;
		//interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
		//	ConfigurationDescriptor,
		//	StartPosition, // StartPosition 
		//	-1,            // InterfaceNumber
		//	0,             // AlternateSetting
		//	-1,            // InterfaceClass
		//	-1,            // InterfaceSubClass
		//	-1);           // InterfaceProtocol

		if (!interfaceDescriptor)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Exit;
		}

		// Set the interface entry
		interfaceList[interfaceIndex].InterfaceDescriptor = interfaceDescriptor;
		interfaceList[interfaceIndex].Interface = NULL;

		// Move the position to the next interface descriptor
		StartPosition = (PUCHAR)interfaceDescriptor + interfaceDescriptor->bLength;

	}

	// Make sure that the InterfaceDescriptor member of the last element to NULL.
	interfaceList[pdx->confDesc->bNumInterfaces].InterfaceDescriptor = NULL;

	// Allocate and build an URB for the select-configuration request.
	status = USBD_SelectConfigUrbAllocateAndBuild(
		pdx->UsbdHandle,
		pdx->confDesc,
		interfaceList,
		&urb);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = SubmitUrbSync(
		pdx,
		urb);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	pdx->confHandle = urb->UrbSelectConfiguration.ConfigurationHandle;

	// Enumerate the pipes in the interface information array, which is now filled with pipe
	// information.

	for (interfaceIndex = 0;
		interfaceIndex < pdx->confDesc->bNumInterfaces;
		interfaceIndex++)
	{
		ULONG i;

		Interface = interfaceList[interfaceIndex].Interface;
		pdx->pipeCount = Interface->NumberOfPipes;
		if (pdx->pipeInfos)
			ExFreePool(pdx->pipeInfos);
		pdx->pipeInfos = (PPIPE_INFO)ExAllocatePool(NonPagedPool, sizeof(PIPE_INFO) * pdx->pipeCount);
		if (!pdx->pipeInfos)
		{
			MyDbgPrint(("allocate pipeinfo failed"));
			goto Exit;
		}

		for (i = 0; i < Interface->NumberOfPipes; i++)
		{
			pdx->pipeInfos[i].address = Interface->Pipes[i].EndpointAddress;
			pdx->pipeInfos[i].handle = Interface->Pipes[i].PipeHandle;
			//if (Interface->Pipes[i].PipeType == UsbdPipeTypeInterrupt)
			//{
			//	pdx->InterruptPipe = pipeHandle;
			//}
			//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_IN(Interface->Pipes[i].EndpointAddress))
			//{
			//	pdx->BulkInPipe = pipeHandle;
			//}
			//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_OUT(Interface->Pipes[i].EndpointAddress))
			//{
			//	pdx->BulkOutPipe = pipeHandle;
			//}
		}
	}

Exit:

	if (interfaceList)
	{
		ExFreePool(interfaceList);
		interfaceList = NULL;
	}

	if (urb)
	{
		USBD_UrbFree(pdx->UsbdHandle, urb);
	}


	return status;

}


NTSTATUS SelectInterface(PDEVICE_EXTENSION pdx, UCHAR altIntNum)
{
	PURB urb = NULL;

	NTSTATUS  status;

	PUSBD_INTERFACE_LIST_ENTRY   interfaceList = NULL;
	PUSB_INTERFACE_DESCRIPTOR    interfaceDescriptor = NULL;
	PUSBD_INTERFACE_INFORMATION  Interface = NULL;

	if (!pdx->confDesc)
	{
		status = GetConfDesc(pdx);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
	}


	// Allocate an array for the list of interfaces
	// The number of elements must be one more than number of interfaces.
	interfaceList = (PUSBD_INTERFACE_LIST_ENTRY)ExAllocatePool(
		NonPagedPool,
		sizeof(USBD_INTERFACE_LIST_ENTRY));

	if (!interfaceList)
	{
		//Failed to allocate memory
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}


	RtlZeroMemory(interfaceList, sizeof(USBD_INTERFACE_LIST_ENTRY));
	// 寻找配用接口描述符  格式化PUSBD_INTERFACE_LIST_ENTRY
	PUCHAR StartPosition = (PUCHAR)pdx->confDesc + pdx->confDesc->bLength;
	PUSB_INTERFACE_DESCRIPTOR temp = (PUSB_INTERFACE_DESCRIPTOR)StartPosition;
	int byteComsumed = pdx->confDesc->bLength;
	while (byteComsumed < pdx->confDesc->wTotalLength)
	{
		if (temp->bAlternateSetting == altIntNum)
		{
			interfaceDescriptor = temp;
			break;
		}
		ULONG size = sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR) * temp->bNumEndpoints;
		byteComsumed += size;
		StartPosition += size;
		temp = (PUSB_INTERFACE_DESCRIPTOR)StartPosition;
	}
	//interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
	//	pdx->confDesc,
	//	StartPosition, // StartPosition 
	//	0,            // InterfaceNumber
	//	altIntNum,    // AlternateSetting
	//	-1,            // InterfaceClass
	//	-1,            // InterfaceSubClass
	//	-1);           // InterfaceProtocol
	if (!interfaceDescriptor)
	{
		MyDbgPrint(("get interfacedesc failed"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}
	// Set the interface entry
	interfaceList->InterfaceDescriptor = interfaceDescriptor;
	interfaceList->Interface = NULL;


	// Allocate and build an URB for the select-configuration request.
	status = USBD_SelectInterfaceUrbAllocateAndBuild(
		pdx->UsbdHandle,
		pdx->confHandle,
		interfaceList,
		&urb);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = SubmitUrbSync(
		pdx,
		urb);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("set interface failed"));
		goto Exit;
	}

	// Enumerate the pipes in the interface information array, which is now filled with pipe
	// information.

	Interface = interfaceList->Interface;
	pdx->pipeCount = Interface->NumberOfPipes;
	if (pdx->pipeInfos)
		ExFreePool(pdx->pipeInfos);
	pdx->pipeInfos = (PPIPE_INFO)ExAllocatePool(NonPagedPool, sizeof(PIPE_INFO) * pdx->pipeCount);
	if (!pdx->pipeInfos)
	{
		MyDbgPrint(("allocate pipeinfo failed"));
		goto Exit;
	}
	for (UCHAR i = 0; i < Interface->NumberOfPipes; i++)
	{
		pdx->pipeInfos[i].address = Interface->Pipes[i].EndpointAddress;
		pdx->pipeInfos[i].handle = Interface->Pipes[i].PipeHandle;
		//if (Interface->Pipes[i].PipeType == UsbdPipeTypeInterrupt)
		//{
		//	pdx->InterruptPipe = pipeHandle;
		//}
		//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_IN(Interface->Pipes[i].EndpointAddress))
		//{
		//	pdx->BulkInPipe = pipeHandle;
		//}
		//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_OUT(Interface->Pipes[i].EndpointAddress))
		//{
		//	pdx->BulkOutPipe = pipeHandle;
		//}
	}

Exit:

	if (interfaceList)
	{
		ExFreePool(interfaceList);
		interfaceList = NULL;
	}

	if (urb)
	{
		USBD_UrbFree(pdx->UsbdHandle, urb);
	}


	return status;

}


NTSTATUS SendNonEP0Data(PDEVICE_EXTENSION pdx, const UCHAR endAddress, PVOID buffer, ULONG bufLen)
{
	PURB urb = NULL;

	NTSTATUS  status;


	// Allocate and build an URB for the select-configuration request.
	status = USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	USBD_PIPE_HANDLE handle = 0;
	for (UCHAR i = 0; i < pdx->pipeCount; i++)
	{
		if (pdx->pipeInfos[i].address == endAddress)
		{
			handle = pdx->pipeInfos[i].handle;
			break;
		}
	}
	if (!handle)
	{
		goto Exit;
	}

	ULONG TransferFlags = 0;
	if (USB_ENDPOINT_DIRECTION_IN(endAddress))
	{
		TransferFlags = USBD_TRANSFER_DIRECTION_IN;
	}
	UsbBuildInterruptOrBulkTransferRequest(urb, sizeof(_URB_BULK_OR_INTERRUPT_TRANSFER), handle, buffer, NULL, bufLen, TransferFlags, NULL);

	status = SubmitUrbSync(
		pdx,
		urb);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("SendNonEP0Data failed"));
		goto Exit;
	}

Exit:


	if (urb)
	{
		USBD_UrbFree(pdx->UsbdHandle, urb);
	}

	return status;
}