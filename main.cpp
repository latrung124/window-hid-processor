#include <iostream>
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <map>

enum class DeviceId {
    Secret01,
};

const std::map<DeviceId, std::pair<USHORT, USHORT>> deviceMap = {
        {DeviceId::Secret01, {0x2B04, 0x1B1C}},
};

class HIDDevice {
public:
    HIDDevice() {}
    ~HIDDevice() {
        closeDevice();
    }

    bool openDevice(const USHORT &productId, const USHORT &vendorId) {
        GUID hidGuid;
        HidD_GetHidGuid(&hidGuid);
        HDEVINFO hardwareDeviceInfo = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (hardwareDeviceInfo == INVALID_HANDLE_VALUE) {
            return false;
        }

        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        int interfaceCount = 0;
        while (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, nullptr, &hidGuid, interfaceCount, &deviceInterfaceData)) { // retrieve all the available interface information.
            std::cout << "interfaceCount: " << interfaceCount << std::endl;
            interfaceCount++;
            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr); // to format interface information for each collection
            PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
            deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (!SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, nullptr, nullptr)) {
                free(deviceInterfaceDetailData);
                continue;
            }

            HANDLE deviceHandle = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (deviceHandle == INVALID_HANDLE_VALUE) {
                free(deviceInterfaceDetailData);
                continue;
            }

            std::cout << "devicePath: " << deviceInterfaceDetailData->DevicePath << std::endl;

            PHIDP_PREPARSED_DATA preparsedData; // retrieve the preparsed data for a specified HID device without having to obtain and interpret a device's entire report descriptor
            if (!HidD_GetPreparsedData(deviceHandle, &preparsedData)) {
                free(deviceInterfaceDetailData);
                CloseHandle(deviceHandle);
                continue;
            }

            std::cout << "preparsedData: " << preparsedData << std::endl;

            HIDP_CAPS capabilities; // retrieve a top-level collection's HIDP_CAPS structure
            // Such as the size, in bytes, of the collection's input, output, and feature reports
            if (!HidP_GetCaps(preparsedData, &capabilities)) {
                free(deviceInterfaceDetailData);
                HidD_FreePreparsedData(preparsedData);
                CloseHandle(deviceHandle);
                continue;
            }

            std::cout << "usagePage: " << capabilities.UsagePage << " usage: " << capabilities.Usage << std::endl;

            if (capabilities.UsagePage == 0x01 && capabilities.Usage == 0x06) { // check if the device is a keyboard
                // do something
                std::cout << "this is a keyboard!" << std::endl;
            }

            HIDD_ATTRIBUTES attributes;
            attributes.Size = sizeof(HIDD_ATTRIBUTES);
            if (HidD_GetAttributes(deviceHandle, &attributes)) {
                std::cout << "vendorId: " << attributes.VendorID << " productId: " << attributes.ProductID << std::endl;
                if (attributes.VendorID == vendorId && attributes.ProductID == productId) {
                    // do something
                    std::cout << "this is a Mosaic keyboard!" << std::endl;
                }
            } else {
                free(deviceInterfaceDetailData);
                HidD_FreePreparsedData(preparsedData);
                CloseHandle(deviceHandle);
                continue;
            }

            free(deviceInterfaceDetailData);
            HidD_FreePreparsedData(preparsedData);

            CloseHandle(deviceHandle);
        }

        //before close device, all data associated with the device should be released
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

        return false;
    }
    void closeDevice() {
    }
};

int main() {
    std::cout << "Hello, Window Hids!" << std::endl;
    HIDDevice device;
    device.openDevice(deviceMap.at(DeviceId::Secret01).first, deviceMap.at(DeviceId::Secret01).second);
    return 0;
}