#pragma once 

//////////////////////////////////////////////////////////////////////////
// CallBack для обнаружения устройств и вывода информации о них на консоль
// 
// cbNumber   - номер CallBack'а
// pvContext  - указатель на DWORD - количество найденных устройств
// pvSpecial1 - указатель на структуру с информацией об устройстве
// pvSpecial2 - ...
//
VOID __stdcall CallBackPrint(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2);

//////////////////////////////////////////////////////////////////////////
// CallBack для поиска указанного пользователем устройства
// 
// cbNumber   - номер CallBack'а
// pvContext  - указатель на FINDCONTEXT
// pvSpecial1 - указатель на структуру с информацией об устройстве
// pvSpecial2 - ...
//
VOID __stdcall CallBackFind(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2);

//
VOID __stdcall CallBackGrabbing(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2);
