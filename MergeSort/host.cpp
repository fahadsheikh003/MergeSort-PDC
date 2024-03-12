#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <CL/cl.hpp>

using namespace std;

#define ARRAY_SIZE 1000

cl::Device* getDevice() {
    vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    vector<cl::Device> allDevices;

    for (auto& p : platforms) {
        vector<cl::Device> devices;
        p.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        for (auto& d : devices) {
            allDevices.push_back(d);
        }

        devices.clear();

        p.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        for (auto& d : devices) {
            allDevices.push_back(d);
        }
    }

    int choice = -1;
    do {
        cout << "Select a device to use from the following list.." << endl << endl;
        for (int i = 0; i < allDevices.size(); i++) {
            auto& d = allDevices[i];
            cout << "Device #: " << i + 1 << endl;
            cout << "Device Name: " << d.getInfo<CL_DEVICE_NAME>() << endl;
            cout << "Device Type: " << (d.getInfo<CL_DEVICE_TYPE>() == 2 ? "CPU" : "GPU") << endl << endl;
        }
        cout << "Enter device #: ";
        cin >> choice;
        cin.ignore();
        cout << endl;
    } while (choice < 1 || choice > allDevices.size());

    return new cl::Device(allDevices[choice - 1]);
}

// Sequential Merge Sort
//void merge(int arr[], int l, int m, int r) {
//    int i, j, k;
//    int n1 = m - l + 1;
//    int n2 = r - m;
//
//    int* L = new int[n1];
//    int* R = new int[n2];
//
//    for (i = 0; i < n1; i++)
//        L[i] = arr[l + i];
//    for (j = 0; j < n2; j++)
//        R[j] = arr[m + 1 + j];
//
//    i = 0;
//    j = 0;
//    k = l;
//    while (i < n1 && j < n2) {
//        if (L[i] <= R[j]) {
//            arr[k] = L[i];
//            i++;
//        }
//        else {
//            arr[k] = R[j];
//            j++;
//        }
//        k++;
//    }
//
//    while (i < n1) {
//        arr[k] = L[i];
//        i++;
//        k++;
//    }
//
//    while (j < n2) {
//        arr[k] = R[j];
//        j++;
//        k++;
//    }
//
//    delete[] L;
//    delete[] R;
//}
//
//void mergeSortSequential(int arr[], int l, int r) {
//    if (l < r) {
//        int m = l + (r - l) / 2;
//
//        mergeSortSequential(arr, l, m);
//        mergeSortSequential(arr, m + 1, r);
//
//        merge(arr, l, m, r);
//    }
//}

void merge(int arr[], int start, int mid, int end)
{
    int start2 = mid + 1;

    // If the direct merge is already sorted
    if (arr[mid] <= arr[start2]) {
        return;
    }

    // Two pointers to maintain start
    // of both arrays to merge
    while (start <= mid && start2 <= end) {

        // If element 1 is in right place
        if (arr[start] <= arr[start2]) {
            start++;
        }
        else {
            int value = arr[start2];
            int index = start2;

            // Shift all the elements between element 1
            // element 2, right by 1.
            while (index != start) {
                arr[index] = arr[index - 1];
                index--;
            }
            arr[start] = value;

            // Update all the pointers
            start++;
            mid++;
            start2++;
        }
    }
}

/* l is for left index and r is right index of the
   sub-array of arr to be sorted */
void mergeSort(int arr[], int l, int r)
{
    if (l < r) {

        // Same as (l + r) / 2, but avoids overflow
        // for large l and r
        int m = l + (r - l) / 2;

        // Sort first and second halves
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);

        merge(arr, l, m, r);
    }
}

int main() {
    srand(time(NULL));
    // Generate random array
    cout << "Initializing Array" << endl;
    int array[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        array[i] = rand() % 1000;
    }
    cout << "Array Initialized" << endl << endl;

    cl::Device* device = getDevice();
    cout << "Selected Device: " << device->getInfo<CL_DEVICE_NAME>() << endl;

    ifstream file("kernel.cl");
    string src(istreambuf_iterator<char>(file), (istreambuf_iterator<char>()));
    cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));

    cl::Context context(*device);
    cl::Program program(context, sources);

    auto err = program.build();
    if (err != CL_BUILD_SUCCESS) {
        std::cerr << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(*device)
            << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device) << std::endl;
        exit(1);
    }

    cl::Buffer buffer(context, CL_MEM_READ_WRITE, sizeof(int) * ARRAY_SIZE);

    cl::Kernel kernel(program, "mergeSort");
    kernel.setArg(0, buffer);
    kernel.setArg(1, 0);
    kernel.setArg(2, ARRAY_SIZE - 1);

    cl::CommandQueue queue(context, *device, CL_QUEUE_PROFILING_ENABLE);
    queue.enqueueWriteBuffer(buffer, CL_TRUE, 0, sizeof(int) * ARRAY_SIZE, array);

    cl::Event _event;
    auto startOpenCL = chrono::high_resolution_clock::now();
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(100), cl::NullRange, NULL, &_event);
    queue.enqueueReadBuffer(buffer, CL_TRUE, 0, sizeof(int) * ARRAY_SIZE, array);
    queue.finish();
    auto endOpenCL = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsedOpenCL = endOpenCL - startOpenCL;

    /*cl_ulong startTime = _event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong endTime = _event.getProfilingInfo<CL_PROFILING_COMMAND_END>();

    cl_ulong elapsedTime = endTime - startTime;

    cout << "Elapsed Time in Parallel Computation: " << elapsedTime << " ns" << endl;*/

    // Sequential Merge Sort
    /*int sequentialArray[ARRAY_SIZE];
    copy(array, array + ARRAY_SIZE, sequentialArray);
    auto startSequential = chrono::high_resolution_clock::now();
    mergeSort(sequentialArray, 0, ARRAY_SIZE - 1);
    auto endSequential = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsedSequential = endSequential - startSequential;*/

    /*for (int i = 0; i < ARRAY_SIZE; ++i) {
        if (sequentialArray[i] != array[i]) {
			cerr << "Error at index " << i << ": " << sequentialArray[i] << " != " << array[i] << endl;
			return 1;
		}
	}*/

    // Print results and compare time
    //cout << "Sequential Merge Sort Time: " << fixed << elapsedSequential.count() << " seconds\n";
    cout << "OpenCL Merge Sort Time: " << fixed << elapsedOpenCL.count() << " seconds\n";

    return 0;
}
