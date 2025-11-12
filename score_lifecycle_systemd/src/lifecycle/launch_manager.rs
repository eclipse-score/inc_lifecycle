use dbus::Path;
use dbus::blocking::Connection;
use std::time::Duration;
use dbus::blocking::Proxy;
use crate::systemd::SystemdUnitInfo;

pub trait ControlAPI {
    fn activate_run_target(&self, name: &str) -> Result<(), Box<dyn std::error::Error>>;
}

pub trait AliveAPI {
    fn report_health_status(&self, name: &str) -> Result<(), Box<dyn std::error::Error>>;
}

pub struct LaunchManagerSD {
    conn: Connection,
}

impl LaunchManagerSD {
    fn proxy(&self) -> Proxy<&Connection> {
        self.conn.with_proxy(
            "org.freedesktop.systemd1",
            "/org/freedesktop/systemd1",
            Duration::from_secs(2),
        )
    }
}

impl Default for LaunchManagerSD {
    fn default() -> Self {
        let conn = Connection::new_system().expect("DBUS connection error");

        Self { conn }
    }
}

impl AliveAPI for LaunchManagerSD {
    fn  report_health_status(&self, name: &str) -> Result<(), Box<dyn std::error::Error>> {
        let (unit_path,): (dbus::Path<'static>,) = self.proxy().method_call(
            "org.freedesktop.systemd1.Manager",
            "GetUnit",
            (name,),
        )?;

        let unit_proxy = &self.conn.with_proxy(
            "org.freedesktop.systemd1",
            unit_path,
            Duration::from_millis(5000),
        );

    
        let info = SystemdUnitInfo::from_unit_proxy(name, &unit_proxy)?;

        match info.active_state.as_str() {
            "active" => Ok(()),
            state => Err(format!("Invalid state: {}", state).into())
        }
    }
}

impl ControlAPI for LaunchManagerSD {
    fn activate_run_target(&self, name: &str) -> Result<(), Box<dyn std::error::Error>> { 
        // Call StartUnit method
        let (_res,): (Path,) =  self.proxy().method_call(
            "org.freedesktop.systemd1.Manager",
            "StartUnit",
            (name, "replace"), // second arg is mode: "replace", "fail", etc.
        )?;

        Ok(())

    }
}
