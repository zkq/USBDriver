

#include "usbdriver.h"


NTSTATUS SendNonEP0Ctl(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter SendNonEP0Ctl"));
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	PVOID sysBuf = pIrp->AssociatedIrp.SystemBuffer;
	PSINGLE_TRANSFER buf = (PSINGLE_TRANSFER)sysBuf;
	PCHAR bufFill = (PCHAR)sysBuf + sizeof(SINGLE_TRANSFER);
	ULONG requireLen = inLen - sizeof(SINGLE_TRANSFER);

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	UCHAR endAddress = buf->ucEndpointAddress;

	status = SendNonEP0Data(pdx, endAddress, bufFill, requireLen);

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? inLen : 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}



NTSTATUS SendEP0Ctl(PDEVICE_EXTENSION pdx, PIRP pIrp)
{

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	PVOID sysBuf = pIrp->AssociatedIrp.SystemBuffer;
	PSINGLE_TRANSFER buf = (PSINGLE_TRANSFER)sysBuf;
	PCHAR bufFill = (PCHAR)sysBuf + sizeof(SINGLE_TRANSFER);
	ULONG requireLen = inLen - sizeof(SINGLE_TRANSFER);
	ULONG realSize = 0;

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	switch (buf->SetupPacket.bRequest){
		//获取描述符
	case USB_REQUEST_GET_DESCRIPTOR:
		switch (buf->SetupPacket.wVal.hiByte){
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
	case USB_REQUEST_SET_CONFIGURATION:
		MyDbgPrint((" set config!!!!"));
		status = SelectConfiguration(pdx);
		break;
	}

	pIrp->IoStatus.Information = NT_SUCCESS(status) ? realSize + sizeof(SINGLE_TRANSFER) : 0;
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
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIRP Irp = IoAllocateIrp((pdx->NextStackDevice->StackSize) + 1, TRUE);
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
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS GetAltIntSetting(PDEVICE_EXTENSION pdx, PIRP pIrp)
{

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ouLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID sysBuf = pIrp->AssociatedIrp.SystemBuffer;
	PCHAR bufFill = (PCHAR)sysBuf;

	MyDbgPrint(("GetAltIntSetting"));

	//URB urb;
	//UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
	//	USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, bufFill, NULL, sizeof(USB_DEVICE_DESCRIPTOR), NULL);
	//status = SubmitUrbSync(pdx, &urb, UrbCompletionRoutine);

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
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;
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


NTSTATUS SendNonEP0Direct(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	MyDbgPrint(("Enter SendNonEP0Direct"));
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
