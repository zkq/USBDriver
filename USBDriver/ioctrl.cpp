

#include "usbdriver.h"


NTSTATUS SendEP0Ctl(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	PUCHAR sysBuf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;
	PSINGLE_TRANSFER single = (PSINGLE_TRANSFER)sysBuf;
	PUCHAR bufFill = sysBuf + single->BufferOffset;
	ULONG requireLen = single->BufferLength;
	ULONG realSize = 0;

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (single->SetupPacket.bmReqType.Type == BMREQUEST_STANDARD)
	{
		switch (single->SetupPacket.bRequest){
			//获取设备状态
		case USB_REQUEST_GET_STATUS:
			status = GetStatus(pdx, (PUSHORT)bufFill, single->SetupPacket.bmReqType.Recipient, single->SetupPacket.wIndex);
			break;
			//清除特征
		case USB_REQUEST_CLEAR_FEATURE:
			status = Feature(pdx, TRUE, single->SetupPacket.bmReqType.Recipient, single->SetupPacket.wValue, single->SetupPacket.wIndex);
			break;
			//设置特征
		case USB_REQUEST_SET_FEATURE:
			status = Feature(pdx, FALSE, single->SetupPacket.bmReqType.Recipient, single->SetupPacket.wValue, single->SetupPacket.wIndex);
			break;
			//设置地址
		case USB_REQUEST_SET_ADDRESS:
			MyDbgPrint((" set address!!!!"));
			status = SetAddress(pdx, single->SetupPacket.wValue);
			break;
			//获取描述符
		case USB_REQUEST_GET_DESCRIPTOR:
			switch (single->SetupPacket.wVal.hiByte){
				//获取设备描述符
			case USB_DEVICE_DESCRIPTOR_TYPE:
				MyDbgPrint((" get device descriptor!!!!"));
				status = GetDeviceDesc(pdx);
				if (NT_SUCCESS(status))
				{
					realSize = requireLen < sizeof(USB_DEVICE_DESCRIPTOR) ? requireLen : sizeof(USB_DEVICE_DESCRIPTOR);
					RtlCopyMemory(bufFill, pdx->deviceDesc, realSize);
				}
				break;
			case USB_CONFIGURATION_DESCRIPTOR_TYPE:
				MyDbgPrint((" get configuration descriptor!!!!"));
				status = GetConfDesc(pdx);
				if (NT_SUCCESS(status))
				{
					realSize = requireLen < pdx->confDesc->wTotalLength ? requireLen : pdx->confDesc->wTotalLength;
					RtlCopyMemory(bufFill, pdx->confDesc, realSize);
				}
				break;
			}
			break;
			//设置描述符
		case USB_REQUEST_SET_DESCRIPTOR:
			MyDbgPrint((" set descriptor!!!!"));
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
			//获取配置
		case USB_REQUEST_GET_CONFIGURATION:
			MyDbgPrint((" get config!!!!"));
			status = GetConfiguration(pdx, bufFill);
			break;
			//设置配置
		case USB_REQUEST_SET_CONFIGURATION:
			MyDbgPrint((" set config!!!!"));
			status = SelectConfiguration(pdx);
			break;
			//获取接口号
		case USB_REQUEST_GET_INTERFACE:
			MyDbgPrint(("get interface!!!!"));
			status = GetInterface(pdx, bufFill);
			break;
			//设置接口
		case USB_REQUEST_SET_INTERFACE:
			MyDbgPrint(("set interface!!!!"));
			UCHAR altNum = single->SetupPacket.wValue;
			status = SelectInterface(pdx, altNum);
			break;
		}
	}
	else if (single->SetupPacket.bmReqType.Type == BMREQUEST_VENDOR)
	{
		MyDbgPrint(("vendor request!!!!"));
		status = VendorRequest(pdx, single);
	}

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? realSize + sizeof(SINGLE_TRANSFER) : 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}



NTSTATUS SendNonEP0Ctl(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter SendNonEP0Ctl"));
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	PUCHAR sysBuf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;
	PSINGLE_TRANSFER single = (PSINGLE_TRANSFER)sysBuf;
	PUCHAR bufFill = sysBuf + single->BufferOffset;
	ULONG requireLen = single->BufferLength;

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	UCHAR endAddress = single->ucEndpointAddress;

	status = SendNonEP0CtlData(pdx, endAddress, 
		sysBuf + single->IsoPacketOffset, single->IsoPacketLength,
		bufFill, requireLen);

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? inLen : 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;

}


NTSTATUS SendNonEP0Direct(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter SendNonEP0Direct"));

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	PUCHAR userInputBuf = (PUCHAR)stack->Parameters.DeviceIoControl.Type3InputBuffer;
	PVOID userOutputBuf = pIrp->UserBuffer;
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	MyDbgPrint(("userinputbuf:0X%0X", userInputBuf));
	MyDbgPrint(("useroutputbuf:0X%0X", userOutputBuf));
	//测试地址是否可读写
	__try
	{
		MyDbgPrint(("enter try block"));
		ProbeForRead(userInputBuf, inLen, sizeof(UCHAR));
		ProbeForWrite(userOutputBuf, outLen, sizeof(UCHAR));

		PSINGLE_TRANSFER singleTrans = (PSINGLE_TRANSFER)userInputBuf;
		UCHAR endAddress = singleTrans->ucEndpointAddress;
		ULONG isoPacketLen = singleTrans->IsoPacketLength;
		PUCHAR isoInfoBuf = userInputBuf + singleTrans->IsoPacketOffset;

		PVOID kernlBuf = ExAllocatePool(NonPagedPool, outLen);
		RtlCopyMemory(kernlBuf, userOutputBuf, outLen);
		status = SendNonEP0CtlData(pdx, endAddress,
			isoInfoBuf, isoPacketLen,
			kernlBuf, outLen);
		if (NT_SUCCESS(status))
		{
			MyDbgPrint(("direct ctl success!!!"))
				RtlCopyMemory(userOutputBuf, kernlBuf, outLen);
		}
		ExFreePool(kernlBuf);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		MyDbgPrint(("catch exception"));
	}

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? outLen : 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}


NTSTATUS GetDriverVersion(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetDriverVersion"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetUsbDIVersion(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetUSBDIVersion"));
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	PULONG version = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
	
	PUSBD_VERSION_INFORMATION info = (PUSBD_VERSION_INFORMATION)ExAllocatePool(NonPagedPool, sizeof(USBD_VERSION_INFORMATION));
	//USBD_GetUSBDIVersion(info);
	*version = info->USBDI_Version;
	ExFreePool(info);

	pIrp->IoStatus.Information = sizeof(ULONG);
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetAltIntSetting(PDEVICE_EXTENSION pdx, PIRP pIrp)
{

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	PVOID sysBuf = pIrp->AssociatedIrp.SystemBuffer;
	PUCHAR bufFill = (PUCHAR)sysBuf;

	MyDbgPrint(("GetAltIntSetting"));
	
	status = GetInterface(pdx, bufFill);

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? inLen : 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS SelIntface(PDEVICE_EXTENSION pdx, PIRP pIrp)
{

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ouLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID sysBuf = pIrp->AssociatedIrp.SystemBuffer;
	PCHAR bufFill = (PCHAR)sysBuf;

	MyDbgPrint(("SelIntface"));
	status = SelectInterface(pdx, bufFill[0]);

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? inLen : 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}



NTSTATUS GetAddress(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetAddress"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetNumEndpoints(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetNumEndpoints"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetPwrState(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetPwrState"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;

}


NTSTATUS SetPwrState(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter SetPwrState"));

	//PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	PUCHAR sysBuf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;
	POWER_STATE state;
	state.DeviceState = (DEVICE_POWER_STATE)sysBuf[0];

	NTSTATUS status = SetPwr(pdx, state);
	pIrp->IoStatus.Information = 1;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


//重新连接USB设备  模拟物理  
NTSTATUS CyclePort(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter CyclePort"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(pIrp);
	stack->Parameters.DeviceIoControl;

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS ResetPipe(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter ResetPipe"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS ResetParentPort(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter ResetParentPort"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetTransSize(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetTransSize"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS SetTransSize(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter SetTransSize"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetDiName(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetDiName"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetFriendlyName(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetFriendlyName"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS AbortPipe(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter AbortPipe"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetSpeed(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetSpeed"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetCurrentFrame(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter GetCurrentFrame"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}
