#!/bin/sh

./lwi_delete_group_members.sh
if [ $? -ne 0 ]; then
   echo "Failed to delete group members"
   exit 1
fi

./lwi_delete_groups.sh
if [ $? -ne 0 ]; then
   echo "Failed to delete groups"
   exit 1
fi

./lwi_delete_users.sh
if [ $? -ne 0 ]; then
   echo "Failed to delete users"
   exit 1
fi

./lwi_populate_groups.sh
if [ $? -ne 0 ]; then
   echo "Failed to populate groups"
   exit 1
fi

./lwi_populate_users.sh
if [ $? -ne 0 ]; then
   echo "Failed to populate users"
   exit 1
fi

./lwi_populate_group_members.sh
if [ $? -ne 0 ]; then
   echo "Failed to populate group members"
   exit 1
fi

