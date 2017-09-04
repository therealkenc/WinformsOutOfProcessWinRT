/**
	This code borrows from the WRLOutOfProcesssWinRTComponenet sample copyright
	Microsoft under a MIT license. Link as of Sept 2017

	https://github.com/Microsoft/DesktopBridgeToUWP-Samples/tree/master/Samples/WinformsOutOfProcessWinRTComponent
 **/
#include <wrl\module.h>
#include <roapi.h>
#include <Windows.ApplicationModel.Core.h>

using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

/*	This is a forward declaration of Platform::Details::InProcModule, which
	is part of (I think) vccorlib, which gets pulled in silently by C++/CX
	(ie, cl.exe flag /ZW). You have to have the WRL Module<T> template
	paramaters match C++/CX or a debug assertion "The module was already instantiated"
	is raised. There is a comment at at line 178 of ...\winrt\wrl\implements.h
	(SDK 16267) which states:

	  WRLs support for activatable classes requires there is only one instance of Module<>, this assert
	  ensures there is only one. Since Module<> is templatized, using different template parameters will
	  result in multiple instances, avoid this by making sure all code in a component uses the same parameters.
	  Note that the C++ CX runtime creates an instance; Module<InProc, Platform::Details::InProcModule>,
	  so mixing it with non CX code can result in this assert.
	  WRL supports static and dynamically allocated Module<>, choose dynamic by defining __WRL_DISABLE_STATIC_INITIALIZE__
	  and allocate that instance with new but only once, for example in the main() entry point of an application.

	Getting the Module<T> correct and defining __WRL_DISABLE_STATIC_INITIALIZE__ makes out of
	process C++/CX components happy. Maybe there is another way, but documentation is scarce.
 */

namespace Platform {
	namespace Details {
		class InProcModule :
			public Microsoft::WRL::Module<Microsoft::WRL::InProcDisableCaching, InProcModule>,
			public __abi_Module
		{
		public:
			InProcModule();
			virtual ~InProcModule();
			virtual unsigned long __stdcall __abi_IncrementObjectCount();
			virtual unsigned long __stdcall __abi_DecrementObjectCount();
		};
	}
} // namespace Platform::Details

class ExeServerGetActivationFactory WrlFinal : public RuntimeClass<IGetActivationFactory, FtmBase>
{
public:
	STDMETHODIMP GetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ IInspectable** factory)
	{
		*factory = nullptr;
		ComPtr<IActivationFactory> activationFactory;
		HRESULT hr = Module<InProc, Platform::Details::InProcModule>::GetModule().GetActivationFactory(activatableClassId, &activationFactory);
		if (SUCCEEDED(hr))
		{
			*factory = activationFactory.Detach();
		}
		return hr;
	}
};

int CALLBACK WinMain(_In_  HINSTANCE, _In_  HINSTANCE, _In_  LPSTR, _In_  int)
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper roInit(RO_INIT_MULTITHREADED);
	if (FAILED(roInit)) return 0;

	ComPtr<ICoreApplication> coreApp;
	if (FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &coreApp))) return 0;

	auto activationFactory = Make<ExeServerGetActivationFactory>();
	if (!activationFactory) return 0;

	coreApp->RunWithActivationFactories(activationFactory.Get());

	return 0;
}
