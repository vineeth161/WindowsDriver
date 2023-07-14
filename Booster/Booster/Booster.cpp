#include <ntddk.h>

/*
* Basic driver operations
* 1) Set an Unload routine	-> BoosterUnload()
* 2) Set dispatch routines the driver supports	-> IRP_MJ_CREATE , IRP_MJ_CLOSE , IRP_MJ_WRITE
* 3) Create a device object
* 4) Create a symbolic link to the device object -> IoCreateSymbolicLink
*/

void BoosterUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS BoosterWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

//DriverEntry

extern "C" NTSTATUS 

DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{ 
	DriverObject->DriverUnload = BoosterUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = BoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = BoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = BoosterWrite;

	/*
		A typical software driver needs one device object , with a symbolic link 
		pointing to it , so that user mode clients can obtain handles easily
		with CreateFile() . Creating the device object requires calling the 
		IoCreateDevice API .
	*/

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Booster"); // L for unicode


	PDEVICE_OBJECT DeviceObject;

	NTSTATUS status = IoCreateDevice(
									DriverObject,
									0,
									&devName,
									FILE_DEVICE_UNKNOWN,
									0,
									FALSE,
									&DeviceObject
									);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status; 
	}

	/*
	*	If all goes well , we now have a pointer to our device object . The next step is 
	*	to make this device onject accessible to user mode callers by providing a 
	*	symbolic link . Creating a symbolic link involves calling IoCreateSymbolicLink
	*
	*	NTSTATUS IoCreateSymbolicLink( PUNICODE_STRING SymbolicLinkName,
	*								   PUNICODE_STRING DeviceName);
	*/

	UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	status = IoCreateSymbolicLink(&symlink, &devName);  //Accepts the sy,bolic link and target of the link
	
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		/*
		*	Note that if the creation fails , we must undo everything done so fat . In this case ,
		*	the device object was created . So undo that by calling IoDeleteDevice().
		*/
		/*
		* Once we have symbolic link and device object seup, DriverEntrty can return success,indicating
		* the driver is now ready to accept requests.	
		*/
		return status;
	}
	return STATUS_SUCCESS;
	/*
	* Before we move on , we must not forget the Unload routine. Assuming DriverEntry completed 
	* succesfully,the Unload routine must undo whatever was done in DriverEntry.In our case , there 
	* are two things to undo : Device object creation and symboli link creation . Undo them in 
	* reverse order
	*/
}

void BoosterUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	KdPrint(("Boster: Driver unload\n"));
	//delete symbolic link
	IoDeleteSymbolicLink(&symLink);
	//delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
}

