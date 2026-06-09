# Iron - Workout Companion for Pebble

Iron is a high-performance, open-source sports tracker for your Pebble smartwatch. Designed for runners and fitness enthusiasts, it tracks your real-time metrics and seamlessly bridges the gap between your Pebble and modern Android health ecosystems.

> ⚠️ **Important Note:** This watch app **will not function at all** unless used in combination with the Android companion app of "Iron".

## 📱 Companion App (Android)

To unlock the full potential of Iron, install the companion Android app. The companion app runs a stable background service during your workout and securely syncs your Pebble's health data directly to **Google Health Connect**.

- **Download on Google Play:** [Iron](https://play.google.com/store/apps/details?id=hag1987haaa.pebble.iron)
- **Source Code (GitHub):** [hag1987haaa/Iron-for-Android](https://github.com/hag1987haaa/Iron-for-Android)

- Zero ads, zero tracking analytics.
- 100% local processing via Bluetooth.
- Only writes data to Health Connect with your explicit permission.

## ✨ Key Features

By combining the Pebble app with the Android companion app, you can unlock a wide range of features:

- **Customizable "Cockpit" Display:** Choose your favorite metrics from 12 different options (Pace, Heart Rate, Cadence, Elevation Gain, Time, etc.) and display them in a massive, easy-to-read font in the center of your watch.
- **Voice Assistant Integration:** Long-press a button on your Pebble to trigger Google Assistant or Gemini (uses your phone's or earbuds' microphone).
- **Smart Notifications:** Get vibration alerts on your wrist for distance laps (e.g., every 1km) or time intervals.
- **"Thin Client" Architecture:** Labels and values are dynamically generated on your phone and sent to your watch. This prevents character encoding issues on Pebble and maximizes visibility.
- **Advanced Data Export:** Supports both GPX and TCX formats. TCX includes heart rate, calories, and smoothed cadence data—perfect for uploading to Strava or Garmin Connect.
- **Detailed Analytics Charts:** View graphs for speed, elevation, heart rate, and steps on your phone. Switch between distance-based and time-based charts.
- **Metric/Imperial Support:** Seamlessly switch between km/kg and mile/lb. Weight settings are automatically converted based on your unit choice.
- **Powerful Automation:** Broadcasts intents based on Pebble interactions or app state changes. Fully compatible with automation apps like Tasker.
- **Health Connect Integration:** Seamlessly syncs your recorded workout data directly to Android's Health Connect.

## ⚙️ Installation

1. Install the watch app via the Rebble Store or Pebble Store (Core Devices).
2. Download and install the [Iron Companion App for Android](https://play.google.com/store/apps/details?id=hag1987haaa.pebble.iron) from Google Play.
3. Grant the necessary permissions (Health Connect & Notifications) in the Android app.
4. Start a workout on your Pebble!

## 🤝 Support the Developer

Iron is completely free and open-source. There are no paywalls or locked features.
If you find this app useful for your daily runs or love the automation integration, consider supporting the project! Your support keeps this project alive and fuels further development.

- ☕ **[Support me on Ko-fi](https://ko-fi.com/1987haaa)**

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

---

# Iron - Pebble用ワークアウトコンパニオン

Ironは、Pebbleスマートウォッチ用の高性能でオープンソースなスポーツトラッカーです。ランナーやフィットネス愛好家のために設計されており、リアルタイムの数値を追跡しながら、Pebbleと最新のAndroidヘルスケアエコシステムをシームレスに連携させます。

> ⚠️ **重要な注意点:** 本ウォッチアプリは、Android版『Iron』アプリとセットで使用しない限り、**一切機能しません**。必ず両方のアプリをインストールしてご利用ください。

## 📱 コンパニオンアプリ (Android)

Ironの真の力を引き出すには、Android用のコンパニオンアプリをインストールしてください。
コンパニオンアプリはワークアウト中に安定したバックグラウンドサービスとして稼働し、Pebbleのヘルスケアデータを**Google Health Connect**へ安全に直接同期します。また、Androidアプリ自体もオープンソースとして公開されています。

*   **Google Playストアでダウンロード:** [Iron](https://play.google.com/store/apps/details?id=hag1987haaa.pebble.iron)
*   **Androidアプリのソースコード (GitHub):** [hag1987haaa/Iron-for-Android](https://github.com/hag1987haaa/Iron-for-Android)

- 広告ゼロ、トラッキング解析ゼロ。
- Bluetooth経由の100%ローカル処理。
- ユーザーの明示的な許可がある場合にのみHealth Connectへデータを書き込みます。

## ✨ 主な機能

PebbleアプリとAndroidコンパニオンアプリを組み合わせることで、以下の多彩な機能を利用できます。

- **カスタマイズ可能な「コックピット」表示:** 12種類のデータ（ペース、心拍、ケイデンス、獲得標高、時計等）から好きな項目を選び、ウォッチ中段に特大表示可能。
- **ボイスアシスタント連携:** Pebbleのボタン長押しでGoogleアシスタントやGeminiを起動。（スマホまたはイヤホンのマイクを使用）
- **スマート通知機能:** 距離ラップ（例：1kmごと）や時間インターバルを振動でお知らせ。
- **「Thin Client」通信設計:** ラベルや数値をスマホ側で動的に生成して送信。Pebble側での文字化けを防ぎ、視認性を最大化。
- **高度なデータ出力:** GPX に加え TCX 出力に対応。心拍数、カロリー、平滑化されたケイデンス情報を含み、StravaやGarmin等へのアップロードに最適。
- **詳細な分析チャート:** 速度、高度、心拍数、歩数をグラフ化。距離ベースと時間ベースの切り替えに対応。
- **メートル/ヤード・ポンド法対応:** km/kg と mile/lb を一括切り替え。単位に合わせて体重設定も自動換算。
- **強力な自動化連携:** Pebbleの操作やアプリの状態変化をインテント送信。Tasker等の自動化アプリと連携可能。
- **Health Connect 完全対応:** ウォッチで記録したデータをAndroidのヘルスコネクトへシームレスに同期。

## ⚙️ インストール方法

1. Rebble StoreまたはPebble Store（Core Devices）からウォッチアプリをインストールします。
2. Google Playから[Android用Ironコンパニオンアプリ](https://play.google.com/store/apps/details?id=hag1987haaa.pebble.iron)をダウンロードしてインストールします。
3. Androidアプリ内で必要な権限（Health Connectと通知）を許可します。
4. Pebbleでワークアウトを開始してください！

## 🤝 開発者をサポートする

Ironは完全に無料でオープンソースです。有料の壁やロックされた機能はありません。
毎日のランニングに役立ったり、自動化アプリとの連携機能を気に入っていただけた場合は、プロジェクトのサポートをご検討ください！皆様の支援がこのプロジェクトを継続・発展させる力になります。

- ☕ **[Ko-fiでサポートする (Buy me a coffee)](https://ko-fi.com/1987haaa)**

## 📝 ライセンス

このプロジェクトはMITライセンスの下で公開されています。詳細はLICENSEファイルをご確認ください。