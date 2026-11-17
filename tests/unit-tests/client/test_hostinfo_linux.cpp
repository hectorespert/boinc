// This file is part of BOINC.
// https://boinc.berkeley.edu
// Copyright (C) 2026 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdio>
#include <string>
#include <filesystem>

#include "gtest/gtest.h"
#include "hostinfo.h"

namespace test_hostinfo_linux {
    class test_hostinfo_linux : public ::testing::Test {};

    TEST_F(test_hostinfo_linux, parse_linux_os_info_lsb_release_ubuntu) {
        const std::filesystem::path fixture_path =
            std::filesystem::current_path().parent_path() /
            "unit-tests" / "testdata" / "lsb-release.ubuntu";

        FILE* lsb_release = fopen(fixture_path.string().c_str(), "r");
        ASSERT_NE(nullptr, lsb_release);

        char os_name[256] = "", os_version[256] = "";
        EXPECT_TRUE(HOST_INFO::parse_linux_os_info(lsb_release, lsbrelease, os_name, sizeof(os_name), os_version, sizeof(os_version)));
        fclose(lsb_release);

        EXPECT_STREQ("Ubuntu", os_name);
        EXPECT_STREQ("Ubuntu 26.04 LTS", os_version);
    }

    TEST_F(test_hostinfo_linux, parse_linux_os_info_os_release_ubuntu) {
        const std::filesystem::path fixture_path =
            std::filesystem::current_path().parent_path() /
            "unit-tests" / "testdata" / "os-release.ubuntu";

        FILE* os_release = fopen(fixture_path.string().c_str(), "r");
        ASSERT_NE(nullptr, os_release);

        char os_name[256] = "", os_version[256] = "";
        EXPECT_TRUE(HOST_INFO::parse_linux_os_info(os_release, osrelease, os_name, sizeof(os_name), os_version, sizeof(os_version)));
        fclose(os_release);

        EXPECT_STREQ("Ubuntu", os_name);
        EXPECT_STREQ("Ubuntu 26.04 LTS", os_version);
    }

    TEST_F(test_hostinfo_linux, parse_linux_os_info_os_release_openwrt) {
        const std::filesystem::path fixture_path =
            std::filesystem::current_path().parent_path() /
            "unit-tests" / "testdata" / "os-release.openwrt";

        FILE* os_release = fopen(fixture_path.string().c_str(), "r");
        ASSERT_NE(nullptr, os_release);

        char os_name[256] = "", os_version[256] = "";
        EXPECT_TRUE(HOST_INFO::parse_linux_os_info(os_release, osrelease, os_name, sizeof(os_name), os_version, sizeof(os_version)));
        fclose(os_release);

        EXPECT_STREQ("OpenWrt", os_name);
        EXPECT_STREQ("OpenWrt 25.12.4", os_version);
    }

    TEST_F(test_hostinfo_linux, arm_cpu_part_name_known) {
        EXPECT_STREQ("Cortex-A53", HOST_INFO::arm_cpu_part_name("0x41", "0xd03"));
        EXPECT_STREQ("Cortex-A72", HOST_INFO::arm_cpu_part_name("0x41", "0xd08"));
        EXPECT_STREQ("Neoverse-N1", HOST_INFO::arm_cpu_part_name("0x41", "0xd0c"));
    }

    TEST_F(test_hostinfo_linux, arm_cpu_part_name_unknown) {
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name("0x41", "0xfff"));
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name("0x51", "0xd03"));
    }

    TEST_F(test_hostinfo_linux, arm_cpu_part_name_invalid) {
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name("", "0xd03"));
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name("0x41", ""));
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name("0x41", "0xd03 x"));
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name(nullptr, "0xd03"));
        EXPECT_EQ(nullptr, HOST_INFO::arm_cpu_part_name("0x41", nullptr));
    }
}
