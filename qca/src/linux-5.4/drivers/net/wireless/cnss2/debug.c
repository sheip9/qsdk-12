/* Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include "main.h"
#include "debug.h"
#include "pci.h"
#include <linux/moduleparam.h>
#define CNSS_IPC_LOG_PAGES		32

void *cnss_ipc_log_context;
void *cnss_ipc_log_long_context;
extern void cnss_dump_qmi_history(void);
struct dentry *cnss_root_dentry;

int log_level = CNSS_LOG_LEVEL_INFO;
EXPORT_SYMBOL(log_level);
module_param(log_level, int, 0644);
MODULE_PARM_DESC(log_level, "CNSS2 Module Log Level");

static int cnss_pin_connect_show(struct seq_file *s, void *data)
{
	struct cnss_plat_data *cnss_priv = s->private;

	seq_puts(s, "Pin connect results\n");
	seq_printf(s, "FW power pin result: %04x\n",
		   cnss_priv->pin_result.fw_pwr_pin_result);
	seq_printf(s, "FW PHY IO pin result: %04x\n",
		   cnss_priv->pin_result.fw_phy_io_pin_result);
	seq_printf(s, "FW RF pin result: %04x\n",
		   cnss_priv->pin_result.fw_rf_pin_result);
	seq_printf(s, "Host pin result: %04x\n",
		   cnss_priv->pin_result.host_pin_result);
	seq_puts(s, "\n");

	return 0;
}

static int cnss_pin_connect_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_pin_connect_show, inode->i_private);
}

static const struct file_operations cnss_pin_connect_fops = {
	.read		= seq_read,
	.release	= single_release,
	.open		= cnss_pin_connect_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static int cnss_stats_show_state(struct seq_file *s,
				 struct cnss_plat_data *plat_priv)
{
	enum cnss_driver_state i;
	int skip = 0;
	unsigned long state;

	seq_printf(s, "\nState: 0x%lx(", plat_priv->driver_state);
	for (i = 0, state = plat_priv->driver_state; state != 0;
	     state >>= 1, i++) {
		if (!(state & 0x1))
			continue;

		if (skip++)
			seq_puts(s, " | ");

		switch (i) {
		case CNSS_QMI_WLFW_CONNECTED:
			seq_puts(s, "QMI_WLFW_CONNECTED");
			continue;
		case CNSS_FW_MEM_READY:
			seq_puts(s, "FW_MEM_READY");
			continue;
		case CNSS_FW_READY:
			seq_puts(s, "FW_READY");
			continue;
		case CNSS_COLD_BOOT_CAL:
			seq_puts(s, "COLD_BOOT_CAL");
			continue;
		case CNSS_DRIVER_LOADING:
			seq_puts(s, "DRIVER_LOADING");
			continue;
		case CNSS_DRIVER_UNLOADING:
			seq_puts(s, "DRIVER_UNLOADING");
			continue;
		case CNSS_DRIVER_IDLE_RESTART:
			seq_puts(s, "IDLE_RESTART");
			continue;
		case CNSS_DRIVER_IDLE_SHUTDOWN:
			seq_puts(s, "IDLE_SHUTDOWN");
			continue;
		case CNSS_DRIVER_PROBED:
			seq_puts(s, "DRIVER_PROBED");
			continue;
		case CNSS_DRIVER_RECOVERY:
			seq_puts(s, "DRIVER_RECOVERY");
			continue;
		case CNSS_FW_BOOT_RECOVERY:
			seq_puts(s, "FW_BOOT_RECOVERY");
			continue;
		case CNSS_DEV_ERR_NOTIFY:
			seq_puts(s, "DEV_ERR");
			continue;
		case CNSS_DRIVER_DEBUG:
			seq_puts(s, "DRIVER_DEBUG");
			continue;
		case CNSS_COEX_CONNECTED:
			seq_puts(s, "COEX_CONNECTED");
			continue;
		case CNSS_IMS_CONNECTED:
			seq_puts(s, "IMS_CONNECTED");
			continue;
		case CNSS_IN_SUSPEND_RESUME:
			seq_puts(s, "IN_SUSPEND_RESUME");
			continue;
		}

		seq_printf(s, "UNKNOWN-%d", i);
	}
	seq_puts(s, ")\n");

	return 0;
}

static int cnss_stats_show(struct seq_file *s, void *data)
{
	struct cnss_plat_data *plat_priv = s->private;

	cnss_stats_show_state(s, plat_priv);

	return 0;
}

static int cnss_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_stats_show, inode->i_private);
}

static const struct file_operations cnss_stats_fops = {
	.read		= seq_read,
	.release	= single_release,
	.open		= cnss_stats_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static ssize_t cnss_dev_boot_debug_write(struct file *fp,
					 const char __user *user_buf,
					 size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	struct cnss_pci_data *pci_priv;
	char buf[64];
	char *cmd;
	unsigned int len = 0;
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	pci_priv = plat_priv->bus_priv;
	if (!pci_priv)
		return -ENODEV;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	cmd = (char *)buf;
	if (sysfs_streq("on", cmd)) {
		ret = cnss_power_on_device(plat_priv, 0);
	} else if (sysfs_streq("off", cmd)) {
		cnss_power_off_device(plat_priv, 0);
	} else if (sysfs_streq("enumerate", cmd)) {
		ret = cnss_pci_init(plat_priv);
	} else if (sysfs_streq("download", cmd)) {
		set_bit(CNSS_DRIVER_DEBUG, &plat_priv->driver_state);
		ret = cnss_pci_start_mhi(pci_priv);
	} else if (sysfs_streq("linkup", cmd)) {
		ret = cnss_resume_pci_link(pci_priv);
	} else if (sysfs_streq("linkdown", cmd)) {
		ret = cnss_suspend_pci_link(pci_priv);
	} else if (sysfs_streq("powerup", cmd)) {
		set_bit(CNSS_DRIVER_DEBUG, &plat_priv->driver_state);
		ret = cnss_driver_event_post(plat_priv,
					     CNSS_DRIVER_EVENT_POWER_UP,
					     CNSS_EVENT_SYNC, NULL);
	} else if (sysfs_streq("shutdown", cmd)) {
		ret = cnss_driver_event_post(plat_priv,
					     CNSS_DRIVER_EVENT_POWER_DOWN,
					     0, NULL);
		clear_bit(CNSS_DRIVER_DEBUG, &plat_priv->driver_state);
	} else if (sysfs_streq("assert", cmd)) {
		ret = cnss_force_fw_assert(&pci_priv->pci_dev->dev);
	} else {
		cnss_pr_err("Device boot debugfs command is invalid\n");
		ret = -EINVAL;
	}

	if (ret)
		return ret;

	return count;
}

static int cnss_dev_boot_debug_show(struct seq_file *s, void *data)
{
	seq_puts(s, "\nUsage: echo <action> > <debugfs_path>/cnss/dev_boot\n");
	seq_puts(s, "<action> can be one of below:\n");
	seq_puts(s, "on: turn on device power, assert WLAN_EN\n");
	seq_puts(s, "off: de-assert WLAN_EN, turn off device power\n");
	seq_puts(s, "enumerate: de-assert PERST, enumerate PCIe\n");
	seq_puts(s, "download: download FW and do QMI handshake with FW\n");
	seq_puts(s, "linkup: bring up PCIe link\n");
	seq_puts(s, "linkdown: bring down PCIe link\n");
	seq_puts(s, "powerup: full power on sequence to boot device, download FW and do QMI handshake with FW\n");
	seq_puts(s, "shutdown: full power off sequence to shutdown device\n");
	seq_puts(s, "assert: trigger firmware assert\n");

	return 0;
}

static int cnss_dev_boot_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_dev_boot_debug_show, inode->i_private);
}

static const struct file_operations cnss_dev_boot_debug_fops = {
	.read		= seq_read,
	.write		= cnss_dev_boot_debug_write,
	.release	= single_release,
	.open		= cnss_dev_boot_debug_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static int cnss_reg_read_debug_show(struct seq_file *s, void *data)
{
	struct cnss_plat_data *plat_priv = s->private;

	mutex_lock(&plat_priv->dev_lock);
	if (!plat_priv->diag_reg_read_buf) {
		seq_puts(s, "\nUsage: echo <mem_type> <offset> <data_len> > <debugfs_path>/cnss/reg_read\n");
		mutex_unlock(&plat_priv->dev_lock);
		return 0;
	}

	seq_printf(s, "\nRegister read, address: 0x%x memory type: 0x%x length: 0x%x\n\n",
		   plat_priv->diag_reg_read_addr,
		   plat_priv->diag_reg_read_mem_type,
		   plat_priv->diag_reg_read_len);

	seq_hex_dump(s, "", DUMP_PREFIX_OFFSET, 32, 4,
		     plat_priv->diag_reg_read_buf,
		     plat_priv->diag_reg_read_len, false);

	plat_priv->diag_reg_read_len = 0;
	kfree(plat_priv->diag_reg_read_buf);
	plat_priv->diag_reg_read_buf = NULL;
	mutex_unlock(&plat_priv->dev_lock);

	return 0;
}

static ssize_t cnss_reg_read_debug_write(struct file *fp,
					 const char __user *user_buf,
					 size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	char buf[64];
	char *sptr, *token;
	unsigned int len = 0;
	u32 reg_offset, mem_type;
	u32 data_len = 0;
	u8 *reg_buf = NULL;
	const char *delim = " ";
	int ret = 0;

	if (!test_bit(CNSS_FW_READY, &plat_priv->driver_state)) {
		cnss_pr_err("Firmware is not ready yet\n");
		return -EINVAL;
	}

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;

	if (!sptr)
		return -EINVAL;

	if (kstrtou32(token, 0, &mem_type))
		return -EINVAL;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;

	if (!sptr)
		return -EINVAL;

	if (kstrtou32(token, 0, &reg_offset))
		return -EINVAL;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;

	if (kstrtou32(token, 0, &data_len))
		return -EINVAL;

	mutex_lock(&plat_priv->dev_lock);
	kfree(plat_priv->diag_reg_read_buf);
	plat_priv->diag_reg_read_buf = NULL;

	reg_buf = kzalloc(data_len, GFP_KERNEL);
	if (!reg_buf) {
		mutex_unlock(&plat_priv->dev_lock);
		return -ENOMEM;
	}

	ret = cnss_wlfw_athdiag_read_send_sync(plat_priv, reg_offset,
					       mem_type, data_len,
					       reg_buf);
	if (ret) {
		kfree(reg_buf);
		mutex_unlock(&plat_priv->dev_lock);
		return ret;
	}

	plat_priv->diag_reg_read_addr = reg_offset;
	plat_priv->diag_reg_read_mem_type = mem_type;
	plat_priv->diag_reg_read_len = data_len;
	plat_priv->diag_reg_read_buf = reg_buf;
	mutex_unlock(&plat_priv->dev_lock);

	return count;
}

static int cnss_reg_read_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_reg_read_debug_show, inode->i_private);
}

static const struct file_operations cnss_reg_read_debug_fops = {
	.read		= seq_read,
	.write		= cnss_reg_read_debug_write,
	.open		= cnss_reg_read_debug_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static int cnss_reg_write_debug_show(struct seq_file *s, void *data)
{
	seq_puts(s, "\nUsage: echo <mem_type> <offset> <reg_val> > <debugfs_path>/cnss/reg_write\n");

	return 0;
}

static ssize_t cnss_reg_write_debug_write(struct file *fp,
					  const char __user *user_buf,
					  size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	char buf[64];
	char *sptr, *token;
	unsigned int len = 0;
	u32 reg_offset, mem_type, reg_val;
	const char *delim = " ";
	int ret = 0;

	if (!test_bit(CNSS_FW_READY, &plat_priv->driver_state)) {
		cnss_pr_err("Firmware is not ready yet\n");
		return -EINVAL;
	}

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;

	if (!sptr)
		return -EINVAL;

	if (kstrtou32(token, 0, &mem_type))
		return -EINVAL;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;

	if (!sptr)
		return -EINVAL;

	if (kstrtou32(token, 0, &reg_offset))
		return -EINVAL;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;

	if (kstrtou32(token, 0, &reg_val))
		return -EINVAL;

	ret = cnss_wlfw_athdiag_write_send_sync(plat_priv, reg_offset, mem_type,
						sizeof(u32),
						(u8 *)&reg_val);
	if (ret)
		return ret;

	return count;
}

static int cnss_reg_write_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_reg_write_debug_show, inode->i_private);
}

static const struct file_operations cnss_reg_write_debug_fops = {
	.read		= seq_read,
	.write		= cnss_reg_write_debug_write,
	.open		= cnss_reg_write_debug_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static ssize_t cnss_runtime_pm_debug_write(struct file *fp,
					   const char __user *user_buf,
					   size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	struct cnss_pci_data *pci_priv;
	char buf[64];
	char *cmd;
	unsigned int len = 0;
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	pci_priv = plat_priv->bus_priv;
	if (!pci_priv)
		return -ENODEV;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	cmd = buf;

	if (sysfs_streq("usage_count", cmd)) {
		cnss_pci_pm_runtime_show_usage_count(pci_priv);
	} else if (sysfs_streq("request_resume", cmd)) {
		ret = cnss_pci_pm_request_resume(pci_priv);
	} else if (sysfs_streq("resume", cmd)) {
		ret = cnss_pci_pm_runtime_resume(pci_priv);
	} else if (sysfs_streq("get", cmd)) {
		ret = cnss_pci_pm_runtime_get(pci_priv);
	} else if (sysfs_streq("get_noresume", cmd)) {
		cnss_pci_pm_runtime_get_noresume(pci_priv);
	} else if (sysfs_streq("put_autosuspend", cmd)) {
		ret = cnss_pci_pm_runtime_put_autosuspend(pci_priv);
	} else if (sysfs_streq("put_noidle", cmd)) {
		cnss_pci_pm_runtime_put_noidle(pci_priv);
	} else if (sysfs_streq("mark_last_busy", cmd)) {
		cnss_pci_pm_runtime_mark_last_busy(pci_priv);
	} else {
		cnss_pr_err("Runtime PM debugfs command is invalid\n");
		ret = -EINVAL;
	}

	if (ret)
		return ret;

	return count;
}

static int cnss_runtime_pm_debug_show(struct seq_file *s, void *data)
{
	seq_puts(s, "\nUsage: echo <action> > <debugfs_path>/cnss/runtime_pm\n");
	seq_puts(s, "<action> can be one of below:\n");
	seq_puts(s, "usage_count: get runtime PM usage count\n");
	seq_puts(s, "get: do runtime PM get\n");
	seq_puts(s, "get_noresume: do runtime PM get noresume\n");
	seq_puts(s, "put_noidle: do runtime PM put noidle\n");
	seq_puts(s, "put_autosuspend: do runtime PM put autosuspend\n");
	seq_puts(s, "mark_last_busy: do runtime PM mark last busy\n");

	return 0;
}

static int cnss_runtime_pm_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_runtime_pm_debug_show, inode->i_private);
}

static const struct file_operations cnss_runtime_pm_debug_fops = {
	.read		= seq_read,
	.write		= cnss_runtime_pm_debug_write,
	.open		= cnss_runtime_pm_debug_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static ssize_t cnss_control_params_debug_write(struct file *fp,
					       const char __user *user_buf,
					       size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	char buf[64];
	char *sptr, *token;
	char *cmd;
	u32 val;
	unsigned int len = 0;
	const char *delim = " ";

	if (!plat_priv)
		return -ENODEV;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;
	if (!sptr)
		return -EINVAL;
	cmd = token;

	token = strsep(&sptr, delim);
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val))
		return -EINVAL;

	if (strcmp(cmd, "quirks") == 0)
		plat_priv->ctrl_params.quirks = val;
	else if (strcmp(cmd, "mhi_timeout") == 0)
		plat_priv->ctrl_params.mhi_timeout = val;
	else if (strcmp(cmd, "qmi_timeout") == 0)
		plat_priv->ctrl_params.qmi_timeout = val;
	else if (strcmp(cmd, "bdf_type") == 0)
		plat_priv->ctrl_params.bdf_type = val;
	else if (strcmp(cmd, "time_sync_period") == 0)
		plat_priv->ctrl_params.time_sync_period = val;
	else
		return -EINVAL;

	return count;
}

static int cnss_show_quirks_state(struct seq_file *s,
				  struct cnss_plat_data *plat_priv)
{
	enum cnss_debug_quirks i;
	int skip = 0;
	unsigned long state;

	seq_printf(s, "quirks: 0x%lx (", plat_priv->ctrl_params.quirks);
	for (i = 0, state = plat_priv->ctrl_params.quirks;
	     state != 0; state >>= 1, i++) {
		if (!(state & 0x1))
			continue;
		if (skip++)
			seq_puts(s, " | ");

		switch (i) {
		case LINK_DOWN_SELF_RECOVERY:
			seq_puts(s, "LINK_DOWN_SELF_RECOVERY");
			continue;
		case SKIP_DEVICE_BOOT:
			seq_puts(s, "SKIP_DEVICE_BOOT");
			continue;
		case USE_CORE_ONLY_FW:
			seq_puts(s, "USE_CORE_ONLY_FW");
			continue;
		case SKIP_RECOVERY:
			seq_puts(s, "SKIP_RECOVERY");
			continue;
		case QMI_BYPASS:
			seq_puts(s, "QMI_BYPASS");
			continue;
		case ENABLE_WALTEST:
			seq_puts(s, "WALTEST");
			continue;
		case ENABLE_PCI_LINK_DOWN_PANIC:
			seq_puts(s, "PCI_LINK_DOWN_PANIC");
			continue;
		case FBC_BYPASS:
			seq_puts(s, "FBC_BYPASS");
			continue;
		case ENABLE_DAEMON_SUPPORT:
			seq_puts(s, "DAEMON_SUPPORT");
			continue;
		case DISABLE_DRV:
			seq_puts(s, "DISABLE_DRV");
			continue;
		}

		seq_printf(s, "UNKNOWN-%d", i);
	}
	seq_puts(s, ")\n");
	return 0;
}

static int cnss_control_params_debug_show(struct seq_file *s, void *data)
{
	struct cnss_plat_data *cnss_priv = s->private;

	seq_puts(s, "\nUsage: echo <params_name> <value> > <debugfs_path>/cnss/control_params\n");
	seq_puts(s, "<params_name> can be one of below:\n");
	seq_puts(s, "quirks: Debug quirks for driver\n");
	seq_puts(s, "mhi_timeout: Timeout for MHI operation in milliseconds\n");
	seq_puts(s, "qmi_timeout: Timeout for QMI message in milliseconds\n");
	seq_puts(s, "bdf_type: Type of board data file to be downloaded\n");
	seq_puts(s, "time_sync_period: Time period to do time sync with device in milliseconds\n");

	seq_puts(s, "\nCurrent value:\n");
	cnss_show_quirks_state(s, cnss_priv);
	seq_printf(s, "mhi_timeout: %u\n", cnss_priv->ctrl_params.mhi_timeout);
	seq_printf(s, "qmi_timeout: %u\n", cnss_priv->ctrl_params.qmi_timeout);
	seq_printf(s, "bdf_type: %u\n", cnss_priv->ctrl_params.bdf_type);
	seq_printf(s, "time_sync_period: %u\n",
		   cnss_priv->ctrl_params.time_sync_period);

	return 0;
}

static int cnss_control_params_debug_open(struct inode *inode,
					  struct file *file)
{
	return single_open(file, cnss_control_params_debug_show,
			   inode->i_private);
}

static const struct file_operations cnss_control_params_debug_fops = {
	.read = seq_read,
	.write = cnss_control_params_debug_write,
	.open = cnss_control_params_debug_open,
	.owner = THIS_MODULE,
	.llseek = seq_lseek,
};

static ssize_t cnss_dynamic_feature_write(struct file *fp,
					  const char __user *user_buf,
					  size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	int ret = 0;
	u64 val;

	ret = kstrtou64_from_user(user_buf, count, 0, &val);
	if (ret)
		return ret;

	plat_priv->dynamic_feature = val;
	ret = cnss_wlfw_dynamic_feature_mask_send_sync(plat_priv);
	if (ret < 0)
		return ret;

	return count;
}

static int cnss_dynamic_feature_show(struct seq_file *s, void *data)
{
	struct cnss_plat_data *cnss_priv = s->private;

	seq_printf(s, "dynamic_feature: 0x%llx\n", cnss_priv->dynamic_feature);

	return 0;
}

static int cnss_dynamic_feature_open(struct inode *inode,
				     struct file *file)
{
	return single_open(file, cnss_dynamic_feature_show,
			   inode->i_private);
}

static const struct file_operations cnss_dynamic_feature_fops = {
	.read = seq_read,
	.write = cnss_dynamic_feature_write,
	.open = cnss_dynamic_feature_open,
	.owner = THIS_MODULE,
	.llseek = seq_lseek,
};

static ssize_t cnss_hds_support_write(struct file *fp,
				      const char __user *user_buf,
				      size_t count, loff_t *off)
{
	struct cnss_plat_data *plat_priv =
		((struct seq_file *)fp->private_data)->private;
	int ret = 0;
	u32 val;

	ret = kstrtou32_from_user(user_buf, count, 0, &val);
	if (ret)
		return ret;

	plat_priv->hds_support = !!val;

	return count;
}

static int cnss_hds_support_show(struct seq_file *s, void *data)
{
	struct cnss_plat_data *plat_priv = s->private;

	seq_printf(s, "hds_support: %s\n",
		   plat_priv->hds_support ? "true" : "false");

	return 0;
}

static int cnss_hds_support_open(struct inode *inode,
				 struct file *file)
{
	return single_open(file, cnss_hds_support_show,
			   inode->i_private);
}

static const struct file_operations cnss_hds_support_fops = {
	.read = seq_read,
	.write = cnss_hds_support_write,
	.open = cnss_hds_support_open,
	.owner = THIS_MODULE,
	.llseek = seq_lseek,
};

static ssize_t cnss_qmi_record_debug_write(struct file *fp,
					   const char __user *user_buf,
					   size_t count, loff_t *off)
{
	char buf[4];

	if (copy_from_user(buf, user_buf, 4))
		return -EFAULT;
	qmi_record(buf[0], 0xD000 | buf[1], buf[2], buf[3]);
	return count;
}

static int cnss_qmi_record_debug_show(struct seq_file *s, void *data)
{
	cnss_dump_qmi_history();
	return 0;
}

static int cnss_qmi_record_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cnss_qmi_record_debug_show, inode->i_private);
}

static const struct file_operations cnss_qmi_record_debug_fops = {
	.read		= seq_read,
	.write		= cnss_qmi_record_debug_write,
	.release	= single_release,
	.open		= cnss_qmi_record_debug_open,
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
};

static int cnss_create_debug_only_node(struct cnss_plat_data *plat_priv)
{
	struct dentry *root_dentry = plat_priv->root_dentry;

	debugfs_create_file("dev_boot", 0600, root_dentry, plat_priv,
			    &cnss_dev_boot_debug_fops);
	debugfs_create_file("reg_read", 0600, root_dentry, plat_priv,
			    &cnss_reg_read_debug_fops);
	debugfs_create_file("reg_write", 0600, root_dentry, plat_priv,
			    &cnss_reg_write_debug_fops);
	debugfs_create_file("runtime_pm", 0600, root_dentry, plat_priv,
			    &cnss_runtime_pm_debug_fops);
	debugfs_create_file("control_params", 0600, root_dentry, plat_priv,
			    &cnss_control_params_debug_fops);
	debugfs_create_file("dynamic_feature", 0600, root_dentry, plat_priv,
			    &cnss_dynamic_feature_fops);
	debugfs_create_file("hds_support", 0600, root_dentry, plat_priv,
			    &cnss_hds_support_fops);

	return 0;
}

static bool debugfs_created;
int cnss_debugfs_create(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct dentry *root_dentry = NULL;

	if (!cnss_root_dentry) {
		cnss_root_dentry = debugfs_create_dir("cnss", 0);
		if (IS_ERR(cnss_root_dentry)) {
			ret = PTR_ERR(cnss_root_dentry);
			cnss_pr_err("Unable to create debugfs %d\n", ret);
			goto out;
		}

		/* Create qmi_record under /sys/kernel/debug/cnss2/ */
		debugfs_create_file("qmi_record", 0600, cnss_root_dentry, NULL,
				    &cnss_qmi_record_debug_fops);
	}

	root_dentry = debugfs_create_dir((char *)&plat_priv->device_name,
					 cnss_root_dentry);
	if (IS_ERR(root_dentry)) {
		ret = PTR_ERR(root_dentry);
		cnss_pr_err("Unable to create debugfs %d\n", ret);
		goto out;
	}
	plat_priv->root_dentry = root_dentry;

	debugfs_create_file("pin_connect_result", 0644, root_dentry, plat_priv,
			    &cnss_pin_connect_fops);
	debugfs_create_file("stats", 0644, root_dentry, plat_priv,
			    &cnss_stats_fops);

	cnss_create_debug_only_node(plat_priv);
	debugfs_created = true;
out:
	return ret;
}

void cnss_debugfs_destroy(struct cnss_plat_data *plat_priv)
{
	debugfs_remove_recursive(cnss_root_dentry);
	cnss_root_dentry = NULL;
	plat_priv->root_dentry = NULL;
}

int cnss_debug_init(void)
{
	cnss_ipc_log_context = ipc_log_context_create(CNSS_IPC_LOG_PAGES,
						      "cnss", 0);
	if (!cnss_ipc_log_context) {
		printk(KERN_ERR "Unable to create IPC log context!\n");
		return -EINVAL;
	}

	cnss_ipc_log_long_context = ipc_log_context_create(CNSS_IPC_LOG_PAGES,
							   "cnss-long", 0);
	if (!cnss_ipc_log_long_context) {
		pr_err("Unable to create IPC long log context\n");
		ipc_log_context_destroy(cnss_ipc_log_context);
		return -EINVAL;
	}

	return 0;
}

void cnss_debug_deinit(void)
{
	if (cnss_ipc_log_long_context) {
		ipc_log_context_destroy(cnss_ipc_log_long_context);
		cnss_ipc_log_long_context = NULL;
	}

	if (cnss_ipc_log_context) {
		ipc_log_context_destroy(cnss_ipc_log_context);
		cnss_ipc_log_context = NULL;
	}
}