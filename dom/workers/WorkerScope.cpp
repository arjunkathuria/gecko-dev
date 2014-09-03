/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WorkerScope.h"

#include "jsapi.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/dom/FunctionBinding.h"
#include "mozilla/dom/DedicatedWorkerGlobalScopeBinding.h"
#include "mozilla/dom/ServiceWorkerGlobalScopeBinding.h"
#include "mozilla/dom/SharedWorkerGlobalScopeBinding.h"
#include "mozilla/dom/Console.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#include "Location.h"
#include "Navigator.h"
#include "Principal.h"
#include "RuntimeService.h"
#include "ScriptLoader.h"
#include "WorkerPrivate.h"
#include "Performance.h"

#define UNWRAP_WORKER_OBJECT(Interface, obj, value)                           \
  UnwrapObject<prototypes::id::Interface##_workers,                           \
    mozilla::dom::Interface##Binding_workers::NativeType>(obj, value)

using namespace mozilla;
using namespace mozilla::dom;
USING_WORKERS_NAMESPACE

BEGIN_WORKERS_NAMESPACE

WorkerGlobalScope::WorkerGlobalScope(WorkerPrivate* aWorkerPrivate)
: mWorkerPrivate(aWorkerPrivate)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  SetIsDOMBinding();
}

WorkerGlobalScope::~WorkerGlobalScope()
{
  mWorkerPrivate->AssertIsOnWorkerThread();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(WorkerGlobalScope)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(WorkerGlobalScope,
                                                  DOMEventTargetHelper)
  tmp->mWorkerPrivate->AssertIsOnWorkerThread();
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mConsole)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPerformance)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLocation)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNavigator)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(WorkerGlobalScope,
                                                DOMEventTargetHelper)
  tmp->mWorkerPrivate->AssertIsOnWorkerThread();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mConsole)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPerformance)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mLocation)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNavigator)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(WorkerGlobalScope,
                                               DOMEventTargetHelper)
  tmp->mWorkerPrivate->AssertIsOnWorkerThread();

  tmp->mWorkerPrivate->TraceTimeouts(aCallbacks, aClosure);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_ADDREF_INHERITED(WorkerGlobalScope, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(WorkerGlobalScope, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WorkerGlobalScope)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalObject)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

JSObject*
WorkerGlobalScope::WrapObject(JSContext* aCx)
{
  MOZ_CRASH("We should never get here!");
}

Console*
WorkerGlobalScope::GetConsole()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!mConsole) {
    mConsole = new Console(nullptr);
  }

  return mConsole;
}

already_AddRefed<WorkerLocation>
WorkerGlobalScope::Location()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!mLocation) {
    WorkerPrivate::LocationInfo& info = mWorkerPrivate->GetLocationInfo();

    mLocation = WorkerLocation::Create(info);
    MOZ_ASSERT(mLocation);
  }

  nsRefPtr<WorkerLocation> location = mLocation;
  return location.forget();
}

already_AddRefed<WorkerNavigator>
WorkerGlobalScope::Navigator()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!mNavigator) {
    mNavigator = WorkerNavigator::Create(mWorkerPrivate->OnLine());
    MOZ_ASSERT(mNavigator);
  }

  nsRefPtr<WorkerNavigator> navigator = mNavigator;
  return navigator.forget();
}

already_AddRefed<WorkerNavigator>
WorkerGlobalScope::GetExistingNavigator() const
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  nsRefPtr<WorkerNavigator> navigator = mNavigator;
  return navigator.forget();
}

void
WorkerGlobalScope::Close(JSContext* aCx)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  mWorkerPrivate->CloseInternal(aCx);
}

OnErrorEventHandlerNonNull*
WorkerGlobalScope::GetOnerror()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  EventListenerManager* elm = GetExistingListenerManager();
  return elm ? elm->GetOnErrorEventHandler() : nullptr;
}

void
WorkerGlobalScope::SetOnerror(OnErrorEventHandlerNonNull* aHandler)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  EventListenerManager* elm = GetOrCreateListenerManager();
  if (elm) {
    elm->SetEventHandler(aHandler);
  }
}

void
WorkerGlobalScope::ImportScripts(JSContext* aCx,
                                 const Sequence<nsString>& aScriptURLs,
                                 ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  scriptloader::Load(aCx, mWorkerPrivate, aScriptURLs, aRv);
}

int32_t
WorkerGlobalScope::SetTimeout(JSContext* aCx,
                              Function& aHandler,
                              const int32_t aTimeout,
                              const Sequence<JS::Value>& aArguments,
                              ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  return mWorkerPrivate->SetTimeout(aCx, &aHandler, EmptyString(), aTimeout,
                                    aArguments, false, aRv);
}

int32_t
WorkerGlobalScope::SetTimeout(JSContext* /* unused */,
                              const nsAString& aHandler,
                              const int32_t aTimeout,
                              const Sequence<JS::Value>& /* unused */,
                              ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  Sequence<JS::Value> dummy;
  return mWorkerPrivate->SetTimeout(GetCurrentThreadJSContext(), nullptr,
                                    aHandler, aTimeout, dummy, false, aRv);
}

void
WorkerGlobalScope::ClearTimeout(int32_t aHandle, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->ClearTimeout(aHandle);
}

int32_t
WorkerGlobalScope::SetInterval(JSContext* aCx,
                               Function& aHandler,
                               const Optional<int32_t>& aTimeout,
                               const Sequence<JS::Value>& aArguments,
                               ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int32_t timeout = aTimeout.WasPassed() ? aTimeout.Value() : 0;

  return mWorkerPrivate->SetTimeout(aCx, &aHandler, EmptyString(), timeout,
                                    aArguments, !!timeout, aRv);
}

int32_t
WorkerGlobalScope::SetInterval(JSContext* /* unused */,
                               const nsAString& aHandler,
                               const Optional<int32_t>& aTimeout,
                               const Sequence<JS::Value>& /* unused */,
                               ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  Sequence<JS::Value> dummy;

  int32_t timeout = aTimeout.WasPassed() ? aTimeout.Value() : 0;

  return mWorkerPrivate->SetTimeout(GetCurrentThreadJSContext(), nullptr,
                                    aHandler, timeout, dummy, !!timeout, aRv);
}

void
WorkerGlobalScope::ClearInterval(int32_t aHandle, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->ClearTimeout(aHandle);
}

void
WorkerGlobalScope::Atob(const nsAString& aAtob, nsAString& aOutput, ErrorResult& aRv) const
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  aRv = nsContentUtils::Atob(aAtob, aOutput);
}

void
WorkerGlobalScope::Btoa(const nsAString& aBtoa, nsAString& aOutput, ErrorResult& aRv) const
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  aRv = nsContentUtils::Btoa(aBtoa, aOutput);
}

void
WorkerGlobalScope::Dump(const Optional<nsAString>& aString) const
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!aString.WasPassed()) {
    return;
  }

  if (!mWorkerPrivate->DumpEnabled()) {
    return;
  }

  NS_ConvertUTF16toUTF8 str(aString.Value());

#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "Gecko", "%s", str.get());
#endif
  fputs(str.get(), stdout);
  fflush(stdout);
}

Performance*
WorkerGlobalScope::GetPerformance()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!mPerformance) {
    mPerformance = new Performance(mWorkerPrivate);
  }

  return mPerformance;
}

DedicatedWorkerGlobalScope::DedicatedWorkerGlobalScope(WorkerPrivate* aWorkerPrivate)
: WorkerGlobalScope(aWorkerPrivate)
{
}

JSObject*
DedicatedWorkerGlobalScope::WrapGlobalObject(JSContext* aCx)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(!mWorkerPrivate->IsSharedWorker());

  JS::CompartmentOptions options;
  mWorkerPrivate->CopyJSCompartmentOptions(options);

  return DedicatedWorkerGlobalScopeBinding_workers::Wrap(aCx, this, this,
                                                         options,
                                                         GetWorkerPrincipal(),
                                                         true);
}

void
DedicatedWorkerGlobalScope::PostMessage(JSContext* aCx,
                                        JS::Handle<JS::Value> aMessage,
                                        const Optional<Sequence<JS::Value>>& aTransferable,
                                        ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->PostMessageToParent(aCx, aMessage, aTransferable, aRv);
}

SharedWorkerGlobalScope::SharedWorkerGlobalScope(WorkerPrivate* aWorkerPrivate,
                                                 const nsCString& aName)
: WorkerGlobalScope(aWorkerPrivate), mName(aName)
{
}

JSObject*
SharedWorkerGlobalScope::WrapGlobalObject(JSContext* aCx)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mWorkerPrivate->IsSharedWorker());

  JS::CompartmentOptions options;
  mWorkerPrivate->CopyJSCompartmentOptions(options);

  return SharedWorkerGlobalScopeBinding_workers::Wrap(aCx, this, this, options,
                                                      GetWorkerPrincipal(),
                                                      true);
}

ServiceWorkerGlobalScope::ServiceWorkerGlobalScope(WorkerPrivate* aWorkerPrivate,
                                                   const nsACString& aScope)
  : WorkerGlobalScope(aWorkerPrivate),
    mScope(NS_ConvertUTF8toUTF16(aScope))
{
}

JSObject*
ServiceWorkerGlobalScope::WrapGlobalObject(JSContext* aCx)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mWorkerPrivate->IsServiceWorker());

  JS::CompartmentOptions options;
  mWorkerPrivate->CopyJSCompartmentOptions(options);

  return ServiceWorkerGlobalScopeBinding_workers::Wrap(aCx, this, this, options,
                                                       GetWorkerPrincipal(),
                                                       true);
}

bool
GetterOnlyJSNative(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
{
  JS_ReportErrorNumber(aCx, js_GetErrorMessage, nullptr, JSMSG_GETTER_ONLY);
  return false;
}

END_WORKERS_NAMESPACE
