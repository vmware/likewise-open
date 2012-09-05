/*
 * Copyright (C) VMware. All rights reserved.
 */

#define LSA_PROVIDER_TAG_SAMPLE "lsa-sample-provider"
#define SAMPLE_DOMAIN "sample.local"

#define BAIL_ON_SAMPLE_ERROR(dwError) \
        if (dwError != LW_ERROR_SUCCESS) \
        { \
            goto error; \
        }

#define LOG_FUNC_ENTER LSA_LOG_INFO("Function enter (%s)", __FUNCTION__)
#define LOG_FUNC_EXIT  LSA_LOG_INFO("Function exit  (%s)", __FUNCTION__)

#define SAMPLE_USER_SHELL "/bin/bash"
