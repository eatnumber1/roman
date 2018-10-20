extern crate treexml;

extern crate hyper;
extern crate hyper_rustls;
extern crate yup_oauth2 as oauth2;
extern crate google_drive3 as drive3;
extern crate serde_json;
extern crate md5;
extern crate hex;

use std::io;
use std::fs;
use std::error;
use std::path::Path;
use std::collections::HashMap;
use std::collections::HashSet;
use std::fmt;
use std::result;
use hex::FromHex;

#[derive(Eq, PartialEq, Hash, Clone)]
struct Rom {
  game_name: String,
  file_name: String,
  size: i64,
  md5: md5::Digest,
}

impl fmt::Debug for Rom {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    write!(f, "Rom {{ game_name: \"{}\", file_name: \"{}\", size: {}, md5: \"{:x}\" }}",
           self.game_name, self.file_name, self.size, self.md5)
  }
}

fn md5_digest(s: &str) -> result::Result<md5::Digest, <[u8; 16] as hex::FromHex>::Error> {
  return Ok(md5::Digest(<[u8; 16]>::from_hex(s)?));
}

// TODO(https://github.com/rahulg/treexml-rs/issues/15): Change this to return a Result when it's
// possible to convert a treexml::Error into a Box<std::error::Error>
fn get_roms() -> HashMap<md5::Digest, Vec<Rom>> {
    let filename = "testdata/NEC - PC Engine CD & TurboGrafx CD - Datfile (421) (2018-08-31 07-40-50).dat";

    let file = fs::OpenOptions::new().read(true).open(filename)
        .expect("Something went wrong reading the file");

    let doc = treexml::Document::parse(file)
        .expect("Something went wrong parsing the file");
    let datafile = doc.root.expect("No root element found");
    let headers = datafile.find_child(|tag| tag.name == "header")
      .expect("Error when looking for header");

    for header in &headers.children {
      let text = header.text.as_ref().expect("Header has no body");
      println!("{}: {}", header.name, text);
    }

    let games = datafile.filter_children(|tag| tag.name == "game");
    let mut romsv: HashMap<md5::Digest, Vec<Rom>> = HashMap::new();
    for game in games {
      let game_name = &game.attributes["name"];
      let roms = game.filter_children(|tag| tag.name == "rom");
      for rom in roms {
        let file_name = &rom.attributes["name"];
        let size = &rom.attributes["size"];
        let md5 = md5_digest(&rom.attributes["md5"]).unwrap();
        let r = Rom {
          game_name: game_name.to_string(),
          file_name: file_name.to_string(),
          size: size.parse().expect(&("Failed to parse size of ".to_owned() + file_name)),
          md5: md5.clone(),
        };
        romsv.entry(md5).or_default().push(r);
      }
    }
    println!("Read {} roms", romsv.len());
    return romsv;
}

struct DriveRomManager {
  roms: HashMap<md5::Digest, Vec<Rom>>,
  rootid: String,
  dest_folder: String,
  hub: drive3::Drive<
    hyper::Client,
    oauth2::Authenticator<
      oauth2::DefaultAuthenticatorDelegate,
      oauth2::DiskTokenStorage,
      hyper::Client>>,
}

type Result = result::Result<(), Box<error::Error>>;

impl DriveRomManager {
  pub fn new(roms: HashMap<md5::Digest, Vec<Rom>>) -> result::Result<DriveRomManager, Box<error::Error>> {
    let secret = oauth2::read_application_secret(Path::new("client_id.json"))?;
    use hyper::net::HttpsConnector;
    use hyper_rustls::TlsClient;
    let auth = oauth2::Authenticator::new(
        &secret, oauth2::DefaultAuthenticatorDelegate,
        hyper::Client::with_connector(HttpsConnector::new(TlsClient::new())),
        oauth2::DiskTokenStorage::new(&"token_store.json".to_string())?,
        Some(oauth2::FlowType::InstalledInteractive));
    let hub = drive3::Drive::new(
        hyper::Client::with_connector(HttpsConnector::new(TlsClient::new())),
        auth);

    return Ok(DriveRomManager {
      roms: roms,
      hub: hub,
      rootid: "1xl1RR4hvIfhlibQuAXBlkDNJHguAivGe".to_string(),
      dest_folder: "1fS8tXRq0_0Fj_uFGIlPpTOQlLynIaBXt".to_string(),
    });
  }

  pub fn organize(&self) -> Result {
    let (mut found_roms, mut created_game_folders) = self.find_already_processed()?;
    self.organize_rec(&self.rootid, &mut found_roms, &mut created_game_folders)?;
    for romv in self.roms.values() {
      for ref rom in romv {
        if !found_roms.contains(rom) {
          println!("Couldn't find {:?}", rom);
        }
      }
    }
    return Ok(());
  }

  fn find_already_processed(&self) -> result::Result<(HashSet<Rom>, HashMap<String, String>), Box<error::Error>> {
    let mut found_roms: HashSet<Rom> = HashSet::new();
    let mut created_game_folders: HashMap<String, String> = HashMap::new();
    self.find_already_processed_rec(&self.dest_folder, None, &mut found_roms, &mut created_game_folders)?;
    return Ok((found_roms, created_game_folders));
  }

  fn find_already_processed_rec(
      &self, dirid: &String, game_name: Option<&String>,
      found_roms: &mut HashSet<Rom>,
      created_game_folders: &mut HashMap<String, String>)
      -> Result {
    let mut next_page_token: Option<String> = None;
    loop {
      let (_, list) = {
        let mut req = self.hub.files().list()
          .q(&format!("'{}' in parents and trashed = false", dirid));
        if next_page_token.is_some() {
          req = req.page_token(&next_page_token.unwrap());
        }
        req.doit()?
      };

      for file in list.files.ok_or("listing not found")? {
        let id = file.id.ok_or("file id not found")?;
        let name = file.name.ok_or("file name not found")?;

        static FOLDER_MIME: &'static str = "application/vnd.google-apps.folder";
        if file.mime_type.map_or(
            false,
            |m| m == FOLDER_MIME) {
          if created_game_folders.contains_key(&name) {
            println!("!! Duplicate folder found: {}", name);
          } else {
            created_game_folders.insert(name.clone(), id.clone());
          }
          self.find_already_processed_rec(&id, Some(&name), found_roms, created_game_folders)?;
          continue;
        }

        if game_name.is_none() {
          panic!("No game name found for file {} in {}", name, dirid);
        }

        let (_, got) = self.hub.files().get(&id)
          .param("fields", "size,md5Checksum")
          .doit()?;
        let size = got.size.ok_or("file size not found")?.parse()?;
        let md5 = md5_digest(&got.md5_checksum.ok_or("file md5 not found")?)?;
        let r = Rom {
          game_name: game_name.unwrap().clone(),
          file_name: name,
          size: size,
          md5: md5,
        };
        if found_roms.contains(&r) {
          println!("!! Duplicate rom found: {:?}\n", r);
          continue;
        }

        if let Some(romv) = self.roms.get(&r.md5) {
          if !romv.contains(&r) {
            println!("!! Unknown file found in destination directory: {:?}", r);
          }
        }
        found_roms.insert(r);
      }

      next_page_token = list.next_page_token;
      if next_page_token.is_none() {
        break;
      }
    }

    return Ok(());
  }

  fn organize_rec(&self, id: &String,
                  found_roms: &mut HashSet<Rom>,
                  created_game_folders: &mut HashMap<String, String>) -> Result {
    let mut next_page_token: Option<String> = None;
    loop {
      let (_, list) = {
        let mut req = self.hub.files().list()
          .q(&format!("'{}' in parents and trashed = false", id));
        if next_page_token.is_some() {
          req = req.page_token(&next_page_token.unwrap());
        }
        req.doit()?
      };

      for file in list.files.ok_or("listing not found")? {
        let id = file.id.ok_or("file id not found")?;
        let name = file.name.ok_or("file name not found")?;

        static FOLDER_MIME: &'static str = "application/vnd.google-apps.folder";
        if file.mime_type.map_or(
            false,
            |m| m == FOLDER_MIME) {
          self.organize_rec(&id, found_roms, created_game_folders)?;
          continue;
        }

        let (_, got) = self.hub.files().get(&id)
          .param("fields", "size,md5Checksum")
          .doit()?;
        let size: i64 = got.size.ok_or("file size not found")?.parse()?;
        let md5 = md5_digest(&got.md5_checksum.ok_or("file md5 not found")?)?;
        if !self.roms.contains_key(&md5) {
          println!("!! File not found: {} with hash {:x}\n", name, md5);
          continue;
        }

        for rom in &self.roms[&md5] {
          if rom.size != size {
            println!("!! File size mismatch: {} != {} for {}\n", rom.size, size, name);
          } else if found_roms.contains(rom) {
            println!("{} is a duplicate of already processed rom {}\n", name, rom.file_name);
          } else {
            found_roms.insert(rom.clone());

            if !created_game_folders.contains_key(&rom.game_name) {
              let mut req = drive3::File::default();
              req.name = Some(rom.game_name.clone());
              req.parents = Some(vec!(self.dest_folder.clone()));
              req.mime_type = Some(FOLDER_MIME.to_string());
              let emptybuf: [u8; 0] = [];
              let empty_stream = io::Cursor::new(&emptybuf);
              let (_, created_folder) = self.hub.files()
                .create(req)
                .upload(empty_stream, FOLDER_MIME.parse().unwrap())?;
              println!("Creating {} in {}\n", rom.game_name, self.dest_folder);
              created_game_folders.insert(rom.game_name.clone(), created_folder.id.unwrap());
            }
            let game_folder_id = &created_game_folders[&rom.game_name];

            println!("{}\n{}\n", name, rom.file_name);

            let mut req = drive3::File::default();
            req.name = Some(rom.file_name.clone());
            req.parents = Some(vec!(game_folder_id.clone()));
            self.hub.files().copy(req, &id).doit()?;
          }
        }
      }

      next_page_token = list.next_page_token;
      if next_page_token.is_none() {
        break;
      }
    }

    return Ok(());
  }
}

fn main() {
  println!("Reading rom DB");
  let roms = get_roms();
  println!("Traversing Drive");
  let drm = DriveRomManager::new(roms).unwrap();
  drm.organize().unwrap();
}

// vim:tabstop=2 shiftwidth=2 softtabstop=2 expandtab textwidth=80
