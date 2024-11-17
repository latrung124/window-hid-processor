#include <iostream>
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <vector>
#include <map>

enum class Protocol {
    Secret01,
};

enum UsagePage : USHORT {
    SecretDevice = 0xff42,
};

enum Usage : USHORT{
    RequestAndReply = 0x01,
    Notification = 0x02,
};

struct DeviceIdenfitier {
    USHORT productId;
    USHORT vendorId;
};

struct CollectionUsage {
    USHORT usagePage;
    USHORT usage;
};

static const std::map<Protocol, std::vector<CollectionUsage>> mDeviceCollectionMap = {
    { Protocol::Secret01, {
        CollectionUsage{ .usagePage = UsagePage::SecretDevice, .usage = Usage::RequestAndReply },
        CollectionUsage{ .usagePage = UsagePage::SecretDevice, .usage = Usage::Notification },
        }
    },
};

class HIDDevice {

    std::vector<HANDLE> deviceHandles;

    USHORT mProductId;
    USHORT mVendorId;
    bool mIsListening;

public:
    HIDDevice(const USHORT &productId, const USHORT &vendorId) : mProductId(productId), mVendorId(vendorId), mIsListening(false) {}
    ~HIDDevice() {
        closeDevice();
    }

    std::vector<HANDLE> getDeviceHandles() {
        return deviceHandles;
    }

    HANDLE getHandleForRequestAndReply() {
        for (auto &deviceHandle : deviceHandles) {
            PHIDP_PREPARSED_DATA preparsedData;
            if (!HidD_GetPreparsedData(deviceHandle, &preparsedData)) {
                continue;
            }

            HIDP_CAPS capabilities;
            if (!HidP_GetCaps(preparsedData, &capabilities)) {
                HidD_FreePreparsedData(preparsedData);
                continue;
            }

            if (capabilities.UsagePage == UsagePage::SecretDevice && capabilities.Usage == Usage::RequestAndReply) {
                std::cout << "RequestAndReply device found!" << std::endl;
                std::cout << "InputReportByteLength: " << capabilities.InputReportByteLength << std::endl;
                std::cout << "OutputReportByteLength: " << capabilities.OutputReportByteLength << std::endl;
                std::cout << "FeatureReportByteLength: " << capabilities.FeatureReportByteLength << std::endl;

                return deviceHandle;
            }
        }
        return nullptr;
    }

    HANDLE getHandleForNotification() {
        for (auto &deviceHandle : deviceHandles) {
            PHIDP_PREPARSED_DATA preparsedData;
            if (!HidD_GetPreparsedData(deviceHandle, &preparsedData)) {
                continue;
            }

            HIDP_CAPS capabilities;
            if (!HidP_GetCaps(preparsedData, &capabilities)) {
                HidD_FreePreparsedData(preparsedData);
                continue;
            }

            if (capabilities.UsagePage == UsagePage::SecretDevice && capabilities.Usage == Usage::Notification) {
                std::cout << "Notification device found!" << std::endl;
                std::cout << "InputReportByteLength: " << capabilities.InputReportByteLength << std::endl;
                std::cout << "OutputReportByteLength: " << capabilities.OutputReportByteLength << std::endl;
                return deviceHandle;
            }
        }
        return nullptr;
    }

    bool openDevice() {
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

            // get usage page and usage id of the device
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
                if (attributes.VendorID == mVendorId && attributes.ProductID == mProductId) {
                    // do something
                    std::cout << "this is a Mosaic keyboard!" << std::endl;
                    deviceHandles.push_back(deviceHandle);
                    free(deviceInterfaceDetailData);
                    continue;
                }
            }

            free(deviceInterfaceDetailData);
            CloseHandle(deviceHandle);
        }

        //before close device, all data associated with the device should be released
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

        return false;
    }
    void closeDevice() {
        for (auto &deviceHandle : deviceHandles) {
            CloseHandle(deviceHandle);
        }
    }
};

int main() {
    std::cout << "Hello, Window Hids!" << std::endl;
    std::map<Protocol, DeviceIdenfitier> mDeviceMap;
    mDeviceMap[Protocol::Secret01] = { .productId = 0x2B04, .vendorId = 0x1B1C };

    HIDDevice device(mDeviceMap[Protocol::Secret01].productId, mDeviceMap[Protocol::Secret01].vendorId);
    device.openDevice();

    std::cout << device.getDeviceHandles().size() << std::endl;
    device.getHandleForRequestAndReply();
    device.getHandleForNotification();

    device.closeDevice();
    return 0;
}