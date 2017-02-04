TARGET:=test
hello_nvme_ywj:
	gcc hello_nvme_ywj.c -L. -llightnvm -o test
test_dev_ywj:
	gcc test_dev_ywj.c -L. -llightnvm -o test -lcunit
test_geo_ywj:
	gcc test_geo_ywj.c -L. -I. -llightnvm -o test
test_addr_ywj:
	gcc test_addr_ywj.c -L. -I. -llightnvm -o $(TARGET)
test_addr_conv_ywj:
	gcc test_addr_conv_ywj.c -L. -I. -llightnvm -o $(TARGET) -lcunit
test_addr_io_ywj:
	gcc test_addr_io_ywj.c -L. -I. -std=c99 -llightnvm -o $(TARGET) -lcunit
test_io_issue_ywj:
	gcc test_io_issue_ywj.c -L. -I. -std=c99 -llightnvm -o $(TARGET)
