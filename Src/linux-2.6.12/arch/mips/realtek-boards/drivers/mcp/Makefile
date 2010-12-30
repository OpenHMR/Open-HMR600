#
# Makefile for the Realtek mcp drivers.
#

mcp_api-objs := mcp_dbg.o mcp_aux.o
mcp_api-objs += mcp_dgst.o mcp_dgst_dev.o aes_h.o sha1.o mcp_buff.o
mcp_api-objs += mcp_cipher.o mcp_cipher_dev.o aes.o des.o tdes.o
mcp_api-objs += mcp_rsa_dev.o rsa.o bi.o

obj-$(CONFIG_REALTEK_MCP_NAPI) += mcp_api.o 
