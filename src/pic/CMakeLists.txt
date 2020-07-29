add_subdirectory(icon)

#set(IMAGE_RCCS res.qrc)
#qt5_add_resources(IMAGE_RCC_SRCS ${IMAGE_RCCS})


set(PNG_DIRS icon)
FOREACH(PNG_DIR ${PNG_DIRS})
    FILE(GLOB IMAGES ${PNG_DIR}/*.*)

ENDFOREACH(PNG_DIR)
