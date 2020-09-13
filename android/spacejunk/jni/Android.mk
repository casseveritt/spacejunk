# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := spacejunk
LOCAL_CFLAGS    := -Werror
LOCAL_ARM_MODE  := arm
MY_APP_ROOT     := ../../../code
MY_APP_INC      := $(LOCAL_PATH)/$(MY_APP_ROOT)
LOCAL_C_INCLUDES := $(MY_APP_INC) $(MY_R3ND_INC) 
LOCAL_CFLAGS    := -DANDROID=1 -DAPP_spacejunklite=1

LOCAL_SRC_FILES := spacejunkNative.cpp

include $(LOCAL_PATH)/app.mk
LOCAL_SRC_FILES +== $(MY_APP_SRC_FILES)

LOCAL_STATIC_LIBRARIES += Regal_static r3_static
LOCAL_LDLIBS    := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(LOCAL_PATH)/../../../../../build/android)
$(call import-add-path, $(LOCAL_PATH)/../../../../../../../src/regal/build/android)
$(call import-module, r3)
$(call import-module, Regal)

