use dbus::Path;
use dbus::blocking::Proxy;
use dbus::blocking::Connection;
use dbus::blocking::stdintf::org_freedesktop_dbus::Properties;

#[derive(Debug)]
pub struct SystemdUnitInfo<'a> {
    // unit name
    pub name: String,
    // unit description
    pub description: String,
    // how the unit is loaded (loaded, not-loaded)
    pub load_state: String,
    // unit active state (active, inactive)
    pub active_state: String,
    // detailed unit state (running, stopped)
    pub sub_state: String,
    // unit file system path
    pub fragment_path: Option<String>,
    // exit code of the main process
    pub exec_main_status: Option<i32>,
    // pid of the main process
    pub exec_main_pid: Option<u32>,
    // values from After=
    pub after: Vec<Path<'a>>,
    // values from Before=
    pub before: Vec<Path<'a>>,
    // values from Requires=
    pub requires: Vec<Path<'a>>,
    // values from Wants=
    pub wants: Vec<Path<'a>>,
}

impl SystemdUnitInfo<'_> {
    pub fn from_unit_proxy(unit_name: &str, proxy: &Proxy<&Connection>) -> Result<Self, Box<dyn std::error::Error>> {
        let description: String = proxy.get("org.freedesktop.systemd1.Unit", "Description")?;
        let load_state: String = proxy.get("org.freedesktop.systemd1.Unit", "LoadState")?;
        let active_state: String = proxy.get("org.freedesktop.systemd1.Unit", "ActiveState")?;
        let sub_state: String = proxy.get("org.freedesktop.systemd1.Unit", "SubState")?;
        let fragment_path: Option<String> = proxy.get("org.freedesktop.systemd1.Unit", "FragmentPath").ok();
        let exec_main_status: Option<i32> = proxy.get("org.freedesktop.systemd1.Service", "ExecMainStatus").ok();
        let exec_main_pid: Option<u32> = proxy.get("org.freedesktop.systemd1.Service", "ExecMainPID").ok();
        let after: Vec<dbus::Path<'static>> = proxy.get("org.freedesktop.systemd1.Unit", "After")?;
        let before: Vec<dbus::Path<'static>> = proxy.get("org.freedesktop.systemd1.Unit", "Before")?;
        let requires: Vec<dbus::Path<'static>> = proxy.get("org.freedesktop.systemd1.Unit", "Requires")?;
        let wants: Vec<dbus::Path<'static>> = proxy.get("org.freedesktop.systemd1.Unit", "Wants")?;

        Ok(Self {
            name: unit_name.to_string(),
            description,
            load_state,
            active_state,
            sub_state,
            fragment_path,
            exec_main_status,
            exec_main_pid,
            after,
            before,
            requires,
            wants,
        })
    }
}
