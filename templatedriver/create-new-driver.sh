#!/bin/bash


usage()
{
    echo "Usage: create-new-driver.sh {driver_name}"
    exit 1
}

if [ "$1" == "" ] ; then
    usage
fi

if [ "$BUILD_ROOT" == "" ] ; then
    echo "ERROR: The script must be run in a standard build environment"
    exit 1
fi

DRIVER_NAME=`echo $1 | tr "[:upper:]" "[:lower:]"`
ROOT_DIR=${BUILD_ROOT}/src/${BUILD_OS_TYPE}
DRIVER_DIR=${ROOT_DIR}/${DRIVER_NAME}

if [ -e "${DRIVER_DIR}" ] ; then
    echo "ERROR: The $DRIVER_DIR directory already exists!"
    exit 1
fi

DRIVER_COMPONENT_FILE="${ROOT_DIR}/build/components/${DRIVER_NAME}.comp"

if [ -e "${DRIVER_COMPONENT_FILE}" ] ; then
    echo "ERROR: The $DRIVER_COMPONENT_FILE file already exists!"
    exit 1
fi

# copy and rename files
rsync -avz --files-from="${ROOT_DIR}/templatedriver/files-list" "${ROOT_DIR}/templatedriver" "${DRIVER_DIR}"
mv "${DRIVER_DIR}/etc/templatedriver.reg.in" "${DRIVER_DIR}/etc/${DRIVER_NAME}.reg.in"

# replace all occurences of "template driver" in the source code
TEMPLATE_FILES=`grep -l -i -R template "${DRIVER_DIR}"`
DRIVER_NAME_CAP=`echo ${DRIVER_NAME} | tr "[:lower:]" "[:upper:]"`
DRIVER_NAME_CAP_FIRST=`echo "${DRIVER_NAME:0:1}" | tr "[:lower:]" "[:upper:]"`${DRIVER_NAME:1}

perl -pi -w -e "s/templatedriver/$DRIVER_NAME/g"           $TEMPLATE_FILES
perl -pi -w -e "s/TemplateDriver/$DRIVER_NAME_CAP_FIRST/g" $TEMPLATE_FILES
perl -pi -w -e "s/TEMPLATEDRIVER/$DRIVER_NAME_CAP/g"       $TEMPLATE_FILES

# integrate into the built system
cp "${ROOT_DIR}/build/components/templatedriver.comp" "${DRIVER_COMPONENT_FILE}"
perl -pi -w -e "s/templatedriver/$DRIVER_NAME/g" "${DRIVER_COMPONENT_FILE}"

perl -pi -w -e "s/^REGFILES=\"(.*)\"/REGFILES=\"\1 $DRIVER_NAME.reg\"/" "${ROOT_DIR}/build/products/lwiso/config"

perl -pi -w -e "s/^PKG_COMPONENTS=\"(.*)\"/PKG_COMPONENTS=\"\1 $DRIVER_NAME\"/" "${ROOT_DIR}/build/packages/lwio/lwio.func"

perl -pi -w -e "s/^\"Load\"=\"(.*)\"/\"Load\"=\"\1,$DRIVER_NAME\"/" "${ROOT_DIR}/lwio/etc/lwiod.reg.in"

perl -pi -w -e "s/^(.*lwio.*libschannel.*dcerpc.*) \\\\$/\1 $DRIVER_NAME \\\\/" "${ROOT_DIR}/MakeKitBuild"
